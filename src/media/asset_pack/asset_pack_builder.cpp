#include "tachyon/media/asset_pack/asset_pack_builder.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

namespace tachyon::media {

namespace {

// ---------------------------------------------------------------------------
// Helper: sanitize a string for use as a filesystem-safe asset ID
// ---------------------------------------------------------------------------
std::string sanitize_asset_id(const std::string& name, std::size_t suffix) {
    std::string id = name.empty() ? "unnamed" : name;
    // Replace anything that isn't alphanumeric, underscore, or hyphen with underscore
    std::string safe;
    safe.reserve(id.size());
    for (char c : id) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            safe += c;
        } else {
            safe += '_';
        }
    }
    // Avoid empty after sanitization
    if (safe.empty()) safe = "unnamed";
    return safe + "_" + std::to_string(suffix);
}

// ---------------------------------------------------------------------------
// Helper: determine file extension based on codec
// ---------------------------------------------------------------------------
std::string mesh_extension(const std::string& codec) {
    if (codec == "draco") return ".draco";
    return ".meshbin";
}

std::string texture_extension(const std::string& codec) {
    if (codec == "basis" || codec == "basisu") return ".basis";
    return ".rgba";
}

// ---------------------------------------------------------------------------
// Helper: robustly read float attributes from a tinygltf accessor
// ---------------------------------------------------------------------------
std::vector<float> read_float_accessor(const tinygltf::Model& model,
                                       int accessor_idx,
                                       int expected_components) {
    if (accessor_idx < 0 || accessor_idx >= static_cast<int>(model.accessors.size())) {
        return {}; // missing attribute
    }

    const auto& acc = model.accessors[accessor_idx];

    // Validate type
    if (acc.type != expected_components) {
        std::cerr << "Error: accessor type mismatch. Expected " << expected_components
                  << " components, got " << acc.type << std::endl;
        return {};
    }

    // Validate component type
    if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        std::cerr << "Error: accessor component type is not FLOAT (got " << acc.componentType << ")" << std::endl;
        return {};
    }

    // Reject sparse accessors
    if (acc.sparse.isSparse) {
        std::cerr << "Error: sparse accessors not supported" << std::endl;
        return {};
    }

    if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) {
        std::cerr << "Error: invalid bufferView in accessor" << std::endl;
        return {};
    }

    const auto& bv = model.bufferViews[acc.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
        std::cerr << "Error: invalid buffer in bufferView" << std::endl;
        return {};
    }

    const auto& buf = model.buffers[bv.buffer];
    std::size_t byte_stride = (bv.byteStride > 0)
                                  ? static_cast<std::size_t>(bv.byteStride)
                                  : static_cast<std::size_t>(acc.type) * sizeof(float);
    std::size_t byte_offset = bv.byteOffset + acc.byteOffset;

    if (byte_offset + acc.count * byte_stride > buf.data.size()) {
        std::cerr << "Error: accessor data exceeds buffer size" << std::endl;
        return {};
    }

    std::vector<float> result;
    result.reserve(acc.count * expected_components);

    for (std::size_t i = 0; i < acc.count; ++i) {
        const float* src = reinterpret_cast<const float*>(&buf.data[byte_offset + i * byte_stride]);
        for (int c = 0; c < expected_components; ++c) {
            result.push_back(src[c]);
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Helper: robustly read indices from a tinygltf accessor
// ---------------------------------------------------------------------------
std::vector<std::uint32_t> read_indices_accessor(const tinygltf::Model& model,
                                                  int accessor_idx) {
    if (accessor_idx < 0 || accessor_idx >= static_cast<int>(model.accessors.size())) {
        return {};
    }

    const auto& acc = model.accessors[accessor_idx];

    if (acc.sparse.isSparse) {
        std::cerr << "Error: sparse index accessor not supported" << std::endl;
        return {};
    }

    if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) {
        std::cerr << "Error: invalid bufferView in index accessor" << std::endl;
        return {};
    }

    const auto& bv = model.bufferViews[acc.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
        std::cerr << "Error: invalid buffer in index bufferView" << std::endl;
        return {};
    }

    const auto& buf = model.buffers[bv.buffer];
    std::size_t byte_offset = bv.byteOffset + acc.byteOffset;

    std::size_t elem_size = 1;
    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        elem_size = 2;
    } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        elem_size = 4;
    } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        elem_size = 1;
    } else {
        std::cerr << "Error: unsupported index component type: " << acc.componentType << std::endl;
        return {};
    }

    if (byte_offset + acc.count * elem_size > buf.data.size()) {
        std::cerr << "Error: index data exceeds buffer size" << std::endl;
        return {};
    }

    std::vector<std::uint32_t> indices;
    indices.reserve(acc.count);

    for (std::size_t i = 0; i < acc.count; ++i) {
        std::size_t off = byte_offset + i * elem_size;
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            indices.push_back(static_cast<std::uint32_t>(buf.data[off]));
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            std::uint16_t val;
            std::memcpy(&val, &buf.data[off], 2);
            indices.push_back(val);
        } else {
            std::uint32_t val;
            std::memcpy(&val, &buf.data[off], 4);
            indices.push_back(val);
        }
    }

    return indices;
}

// ---------------------------------------------------------------------------
// Codec selection with strict error on unavailable codec
// ---------------------------------------------------------------------------
MeshCompressor& select_mesh_compressor(const std::string& codec, std::string& error) {
    if (codec == "none") {
        return none_mesh_compressor();
    }
#if defined(TACHYON_ENABLE_DRACO)
    if (codec == "draco") {
        return draco_mesh_compressor();
    }
#endif
    if (codec == "draco") {
        error = "Mesh codec 'draco' was requested but TACHYON_ENABLE_DRACO is not enabled";
    } else {
        error = "Unknown mesh codec: " + codec;
    }
    return none_mesh_compressor(); // caller checks error string
}

TextureCompressor& select_texture_compressor(const std::string& codec, std::string& error) {
    if (codec == "none") {
        return none_texture_compressor();
    }
#if defined(TACHYON_ENABLE_BASIS)
    if (codec == "basisu" || codec == "basis") {
        return basis_texture_compressor();
    }
#endif
    if (codec == "basis" || codec == "basisu") {
        error = "Texture codec '" + codec + "' was requested but TACHYON_ENABLE_BASIS is not enabled";
    } else {
        error = "Unknown texture codec: " + codec;
    }
    return none_texture_compressor();
}

// ---------------------------------------------------------------------------
// Helper: write a blob to disk, return false on failure
// ---------------------------------------------------------------------------
bool write_blob(const std::filesystem::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        std::cerr << "Error: cannot open file for writing: " << path << std::endl;
        return false;
    }
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!ofs) {
        std::cerr << "Error: failed to write " << data.size() << " bytes to " << path << std::endl;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Helper: convert image data to RGBA
// ---------------------------------------------------------------------------
std::vector<std::uint8_t> convert_to_rgba(const std::vector<std::uint8_t>& src,
                                           int width, int height, int channels) {
    if (channels == 4) {
        return src; // already RGBA
    }

    std::size_t num_pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<std::uint8_t> rgba(num_pixels * 4, 255);

    for (std::size_t i = 0; i < num_pixels; ++i) {
        std::size_t src_idx = i * channels;
        std::size_t dst_idx = i * 4;

        switch (channels) {
            case 1: // Grayscale -> R=G=B=gray, A=255
                rgba[dst_idx]     = src[src_idx];
                rgba[dst_idx + 1] = src[src_idx];
                rgba[dst_idx + 2] = src[src_idx];
                rgba[dst_idx + 3] = 255;
                break;
            case 2: // GA -> R=G=B=gray, A=alpha
                rgba[dst_idx]     = src[src_idx];
                rgba[dst_idx + 1] = src[src_idx];
                rgba[dst_idx + 2] = src[src_idx];
                rgba[dst_idx + 3] = src[src_idx + 1];
                break;
            case 3: // RGB -> RGBA
                rgba[dst_idx]     = src[src_idx];
                rgba[dst_idx + 1] = src[src_idx + 1];
                rgba[dst_idx + 2] = src[src_idx + 2];
                rgba[dst_idx + 3] = 255;
                break;
            default:
                break;
        }
    }

    return rgba;
}

} // namespace

AssetPackBuilderResult AssetPackBuilder::build(const AssetPackBuilderOptions& options) {
    AssetPackBuilderResult result;

    try {
        std::filesystem::path out_dir = options.output_dir;
        std::filesystem::create_directories(out_dir);
        std::filesystem::create_directories(out_dir / "meshes");
        std::filesystem::create_directories(out_dir / "textures");
        std::filesystem::create_directories(out_dir / "metadata");

        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret = false;
        std::filesystem::path input_path(options.input_path);
        if (input_path.extension() == ".glb" || input_path.extension() == ".GLB") {
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, options.input_path);
        } else {
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, options.input_path);
        }

        if (!ret) {
            result.ok = false;
            result.error = "Failed to load GLTF: " + err;
            return result;
        }

        // Select compressors with strict error checking
        std::string mesh_error, tex_error;
        MeshCompressor& mesh_comp = select_mesh_compressor(options.mesh_codec, mesh_error);
        if (!mesh_error.empty()) {
            result.ok = false;
            result.error = mesh_error;
            return result;
        }

        TextureCompressor& tex_comp = select_texture_compressor(options.texture_codec, tex_error);
        if (!tex_error.empty()) {
            result.ok = false;
            result.error = tex_error;
            return result;
        }

        // 0. Pre-generate sanitized texture IDs
        std::vector<std::string> texture_id_map;
        for (size_t i = 0; i < model.images.size(); ++i) {
            const auto& image = model.images[i];
            std::string id = sanitize_asset_id(image.name, i);
            texture_id_map.push_back(id);
        }

        // 1. Process Meshes
        for (size_t i = 0; i < model.meshes.size(); ++i) {
            const auto& mesh = model.meshes[i];
            for (size_t j = 0; j < mesh.primitives.size(); ++j) {
                const auto& primitive = mesh.primitives[j];
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;

                MeshCompressionInput minput;

                // Extract positions (VEC3 FLOAT)
                if (primitive.attributes.count("POSITION")) {
                    minput.positions = read_float_accessor(
                        model, primitive.attributes.at("POSITION"), TINYGLTF_TYPE_VEC3);
                    if (minput.positions.empty()) {
                        result.ok = false;
                        result.error = "Failed to read POSITION accessor";
                        return result;
                    }
                }

                // Extract normals (VEC3 FLOAT)
                if (primitive.attributes.count("NORMAL")) {
                    minput.normals = read_float_accessor(
                        model, primitive.attributes.at("NORMAL"), TINYGLTF_TYPE_VEC3);
                }

                // Extract UVs (VEC2 FLOAT)
                if (primitive.attributes.count("TEXCOORD_0")) {
                    minput.uvs = read_float_accessor(
                        model, primitive.attributes.at("TEXCOORD_0"), TINYGLTF_TYPE_VEC2);
                }

                // Extract indices with robust reader
                if (primitive.indices >= 0) {
                    minput.indices = read_indices_accessor(model, primitive.indices);
                }

                auto moutput = mesh_comp.compress(minput);

                PackedMeshEntry entry;
                entry.id = sanitize_asset_id(mesh.name, i * 1000 + j);
                entry.source_path = options.input_path + "#mesh" + std::to_string(i) + "prim" + std::to_string(j);
                entry.codec = moutput.codec;
                entry.source_bytes = minput.positions.size() * sizeof(float)
                                   + minput.normals.size() * sizeof(float)
                                   + minput.uvs.size() * sizeof(float)
                                   + minput.indices.size() * sizeof(uint32_t);
                entry.packed_bytes = moutput.bytes.size();

                // Use proper extension per codec
                entry.packed_path = "meshes/" + entry.id + mesh_extension(moutput.codec);

                // Handle texture dependencies
                if (primitive.material >= 0 && primitive.material < static_cast<int>(model.materials.size())) {
                    const auto& mat = model.materials[primitive.material];

                    auto add_texture = [&](const std::string& key) {
                        auto it = mat.values.find(key);
                        if (it != mat.values.end()) {
                            int tex_idx = it->second.TextureIndex();
                            if (tex_idx >= 0 && tex_idx < static_cast<int>(model.textures.size())) {
                                int img_idx = model.textures[tex_idx].source;
                                if (img_idx >= 0 && img_idx < static_cast<int>(texture_id_map.size())) {
                                    entry.texture_ids.push_back(texture_id_map[img_idx]);
                                }
                            }
                        }
                    };

                    add_texture("baseColorTexture");
                    add_texture("metallicRoughnessTexture");
                }

                // Write mesh data to disk
                if (!write_blob(out_dir / entry.packed_path, moutput.bytes)) {
                    result.ok = false;
                    result.error = "Failed to write mesh: " + entry.packed_path;
                    return result;
                }

                result.manifest.meshes.push_back(entry);
            }
        }

        // 2. Process Textures
        for (size_t i = 0; i < model.images.size(); ++i) {
            const auto& image = model.images[i];

            // Normalize to RGBA before compression
            std::vector<std::uint8_t> rgba = convert_to_rgba(
                image.image, image.width, image.height, image.component);

            TextureCompressionInput tinput;
            tinput.width = image.width;
            tinput.height = image.height;
            tinput.channels = 4; // always RGBA now
            tinput.rgba = std::move(rgba);

            auto toutput = tex_comp.compress(tinput);

            PackedTextureEntry entry;
            entry.id = sanitize_asset_id(image.name, i);
            entry.source_path = options.input_path + "#image" + std::to_string(i);
            entry.codec = toutput.codec;
            entry.source_bytes = tinput.rgba.size();
            entry.packed_bytes = toutput.bytes.size();

            // Use proper extension per codec
            entry.packed_path = "textures/" + entry.id + texture_extension(toutput.codec);

            // Write texture data to disk
            if (!write_blob(out_dir / entry.packed_path, toutput.bytes)) {
                result.ok = false;
                result.error = "Failed to write texture: " + entry.packed_path;
                return result;
            }

            result.manifest.textures.push_back(entry);
        }

        // Write the manifest
        if (!write_asset_pack_manifest(result.manifest, (out_dir / "manifest.json").string())) {
            result.ok = false;
            result.error = "Failed to write manifest";
            return result;
        }

        result.ok = true;
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
    }

    return result;
}

} // namespace tachyon::media