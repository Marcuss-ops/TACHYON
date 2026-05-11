#include "tachyon/media/asset_pack/asset_pack_builder.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

namespace tachyon::media {

namespace {

MeshCompressor& select_mesh_compressor(const std::string& codec) {
#if defined(TACHYON_ENABLE_DRACO)
    if (codec == "draco") {
        return draco_mesh_compressor();
    }
#endif
    if (codec != "none") {
        std::cerr << "Warning: mesh codec " << codec << " requested but not enabled. Falling back to 'none'." << std::endl;
    }
    return none_mesh_compressor();
}

TextureCompressor& select_texture_compressor(const std::string& codec) {
#if defined(TACHYON_ENABLE_BASIS)
    if (codec == "basisu" || codec == "basis") {
        return basis_texture_compressor();
    }
#endif
    if (codec != "none") {
        std::cerr << "Warning: texture codec " << codec << " requested but not enabled. Falling back to 'none'." << std::endl;
    }
    return none_texture_compressor();
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

        MeshCompressor& mesh_comp = select_mesh_compressor(options.mesh_codec);
        TextureCompressor& tex_comp = select_texture_compressor(options.texture_codec);

        // 0. Pre-generate texture IDs map
        std::vector<std::string> texture_id_map;
        for (size_t i = 0; i < model.images.size(); ++i) {
            const auto& image = model.images[i];
            std::string id = image.name.empty() ? "tex_" + std::to_string(i) : image.name;
            texture_id_map.push_back(id);
        }

        // 1. Process Meshes
        for (size_t i = 0; i < model.meshes.size(); ++i) {
            const auto& mesh = model.meshes[i];
            for (size_t j = 0; j < mesh.primitives.size(); ++j) {
                const auto& primitive = mesh.primitives[j];
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;

                MeshCompressionInput minput;
                
                // Extract positions
                if (primitive.attributes.count("POSITION")) {
                    const auto& acc = model.accessors[primitive.attributes.at("POSITION")];
                    const auto& bv = model.bufferViews[acc.bufferView];
                    const float* data = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset]);
                    minput.positions.assign(data, data + acc.count * 3);
                }

                // Extract normals
                if (primitive.attributes.count("NORMAL")) {
                    const auto& acc = model.accessors[primitive.attributes.at("NORMAL")];
                    const auto& bv = model.bufferViews[acc.bufferView];
                    const float* data = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset]);
                    minput.normals.assign(data, data + acc.count * 3);
                }

                // Extract UVs
                if (primitive.attributes.count("TEXCOORD_0")) {
                    const auto& acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                    const auto& bv = model.bufferViews[acc.bufferView];
                    const float* data = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset]);
                    minput.uvs.assign(data, data + acc.count * 2);
                }

                // Extract indices
                if (primitive.indices >= 0) {
                    const auto& acc = model.accessors[primitive.indices];
                    const auto& bv = model.bufferViews[acc.bufferView];
                    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        const uint16_t* data = reinterpret_cast<const uint16_t*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset]);
                        minput.indices.assign(data, data + acc.count);
                    } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                        const uint32_t* data = reinterpret_cast<const uint32_t*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset]);
                        minput.indices.assign(data, data + acc.count);
                    }
                }

                auto moutput = mesh_comp.compress(minput);
                
                PackedMeshEntry entry;
                entry.id = mesh.name.empty() ? "mesh_" + std::to_string(i) + "_" + std::to_string(j) : mesh.name + "_" + std::to_string(j);
                entry.source_path = options.input_path + "#mesh" + std::to_string(i) + "prim" + std::to_string(j);
                entry.packed_path = "meshes/" + entry.id + ".pack";
                entry.codec = moutput.codec;
                entry.source_bytes = minput.positions.size() * sizeof(float) + minput.normals.size() * sizeof(float) + minput.uvs.size() * sizeof(float) + minput.indices.size() * sizeof(uint32_t);
                entry.packed_bytes = moutput.bytes.size();

                // Handle dependencies
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

                std::ofstream ofs(out_dir / entry.packed_path, std::ios::binary);
                ofs.write(reinterpret_cast<const char*>(moutput.bytes.data()), moutput.bytes.size());
                
                result.manifest.meshes.push_back(entry);
            }
        }

        // 2. Process Textures
        for (size_t i = 0; i < model.images.size(); ++i) {
            const auto& image = model.images[i];
            
            TextureCompressionInput tinput;
            tinput.width = image.width;
            tinput.height = image.height;
            tinput.channels = image.component;
            tinput.rgba = image.image;

            auto toutput = tex_comp.compress(tinput);

            PackedTextureEntry entry;
            entry.id = image.name.empty() ? "tex_" + std::to_string(i) : image.name;
            entry.source_path = options.input_path + "#image" + std::to_string(i);
            entry.packed_path = "textures/" + entry.id + ".pack";
            entry.codec = toutput.codec;
            entry.source_bytes = tinput.rgba.size();
            entry.packed_bytes = toutput.bytes.size();

            std::ofstream ofs(out_dir / entry.packed_path, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(toutput.bytes.data()), toutput.bytes.size());

            result.manifest.textures.push_back(entry);
        }

        // Write the manifest
        write_asset_pack_manifest(result.manifest, (out_dir / "manifest.json").string());

        result.ok = true;
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
    }

    return result;
}

} // namespace tachyon::media