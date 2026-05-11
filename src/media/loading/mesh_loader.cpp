#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "tachyon/media/loading/mesh_loader.h"

#include <cstring>
#include <iostream>

namespace tachyon::media {

namespace {

int component_count_from_gltf_type(int type) {
    switch (type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2:   return 2;
        case TINYGLTF_TYPE_VEC3:   return 3;
        case TINYGLTF_TYPE_VEC4:   return 4;
        case TINYGLTF_TYPE_MAT2:   return 4;
        case TINYGLTF_TYPE_MAT3:   return 9;
        case TINYGLTF_TYPE_MAT4:   return 16;
        default: return 0;
    }
}

template <typename T>
bool read_scalar(const std::vector<unsigned char>& data, std::size_t offset, T& out) {
    if (offset + sizeof(T) > data.size()) {
        return false;
    }

    std::memcpy(&out, data.data() + offset, sizeof(T));
    return true;
}

std::vector<float> read_float_accessor(const tinygltf::Model& model, int accessor_index) {
    if (accessor_index < 0 || accessor_index >= static_cast<int>(model.accessors.size())) {
        return {};
    }

    const auto& accessor = model.accessors[accessor_index];
    const int component_count = component_count_from_gltf_type(accessor.type);
    if (component_count <= 0 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.sparse.isSparse) {
        return {};
    }
    if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size())) {
        return {};
    }

    const auto& buffer_view = model.bufferViews[accessor.bufferView];
    if (buffer_view.buffer < 0 || buffer_view.buffer >= static_cast<int>(model.buffers.size())) {
        return {};
    }

    const auto& buffer = model.buffers[buffer_view.buffer];
    const std::size_t stride = buffer_view.byteStride > 0
        ? static_cast<std::size_t>(buffer_view.byteStride)
        : static_cast<std::size_t>(component_count) * sizeof(float);
    const std::size_t base_offset = static_cast<std::size_t>(buffer_view.byteOffset + accessor.byteOffset);

    if (accessor.count == 0) {
        return {};
    }

    const std::size_t required_bytes = base_offset
        + (static_cast<std::size_t>(accessor.count - 1) * stride)
        + static_cast<std::size_t>(component_count) * sizeof(float);
    if (required_bytes > buffer.data.size()) {
        return {};
    }

    std::vector<float> result;
    result.reserve(static_cast<std::size_t>(accessor.count) * static_cast<std::size_t>(component_count));

    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t item_offset = base_offset + i * stride;
        for (int c = 0; c < component_count; ++c) {
            float value{};
            if (!read_scalar(buffer.data, item_offset + static_cast<std::size_t>(c) * sizeof(float), value)) {
                return {};
            }
            result.push_back(value);
        }
    }

    return result;
}

std::vector<std::uint32_t> read_uint_accessor(const tinygltf::Model& model, int accessor_index) {
    if (accessor_index < 0 || accessor_index >= static_cast<int>(model.accessors.size())) {
        return {};
    }

    const auto& accessor = model.accessors[accessor_index];
    const int component_count = component_count_from_gltf_type(accessor.type);
    if (component_count <= 0 || accessor.sparse.isSparse || accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size())) {
        return {};
    }

    const auto& buffer_view = model.bufferViews[accessor.bufferView];
    if (buffer_view.buffer < 0 || buffer_view.buffer >= static_cast<int>(model.buffers.size())) {
        return {};
    }

    std::size_t component_size = 0;
    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: component_size = sizeof(std::uint8_t); break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: component_size = sizeof(std::uint16_t); break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: component_size = sizeof(std::uint32_t); break;
        default: return {};
    }

    const auto& buffer = model.buffers[buffer_view.buffer];
    const std::size_t stride = buffer_view.byteStride > 0
        ? static_cast<std::size_t>(buffer_view.byteStride)
        : static_cast<std::size_t>(component_count) * component_size;
    const std::size_t base_offset = static_cast<std::size_t>(buffer_view.byteOffset + accessor.byteOffset);

    if (accessor.count == 0) {
        return {};
    }

    const std::size_t required_bytes = base_offset
        + (static_cast<std::size_t>(accessor.count - 1) * stride)
        + static_cast<std::size_t>(component_count) * component_size;
    if (required_bytes > buffer.data.size()) {
        return {};
    }

    std::vector<std::uint32_t> result;
    result.reserve(static_cast<std::size_t>(accessor.count) * static_cast<std::size_t>(component_count));

    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t item_offset = base_offset + i * stride;
        for (int c = 0; c < component_count; ++c) {
            const std::size_t component_offset = item_offset + static_cast<std::size_t>(c) * component_size;
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                    result.push_back(static_cast<std::uint32_t>(buffer.data[component_offset]));
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    std::uint16_t value{};
                    if (!read_scalar(buffer.data, component_offset, value)) {
                        return {};
                    }
                    result.push_back(static_cast<std::uint32_t>(value));
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                    std::uint32_t value{};
                    if (!read_scalar(buffer.data, component_offset, value)) {
                        return {};
                    }
                    result.push_back(value);
                    break;
                }
                default:
                    return {};
            }
        }
    }

    return result;
}

std::vector<std::uint32_t> read_index_accessor(const tinygltf::Model& model, int accessor_index) {
    return read_uint_accessor(model, accessor_index);
}

} // namespace

static void process_node(
    const tinygltf::Model& model,
    const tinygltf::Node& node,
    const math::Matrix4x4& parent_transform,
    MeshAsset* asset) {

    math::Matrix4x4 local_transform = math::Matrix4x4::identity();

    if (node.matrix.size() == 16) {
        float matrix_data[16];
        for (int i = 0; i < 16; ++i) {
            matrix_data[i] = static_cast<float>(node.matrix[i]);
        }
        local_transform = math::Matrix4x4(matrix_data);
    } else {
        math::Matrix4x4 t = math::Matrix4x4::identity();
        math::Matrix4x4 r = math::Matrix4x4::identity();
        math::Matrix4x4 s = math::Matrix4x4::identity();

        if (node.translation.size() == 3) {
            t.data[12] = static_cast<float>(node.translation[0]);
            t.data[13] = static_cast<float>(node.translation[1]);
            t.data[14] = static_cast<float>(node.translation[2]);
        }
        if (node.rotation.size() == 4) {
            const float x = static_cast<float>(node.rotation[0]);
            const float y = static_cast<float>(node.rotation[1]);
            const float z = static_cast<float>(node.rotation[2]);
            const float w = static_cast<float>(node.rotation[3]);
            const float x2 = x + x;
            const float y2 = y + y;
            const float z2 = z + z;
            const float xx = x * x2;
            const float xy = x * y2;
            const float xz = x * z2;
            const float yy = y * y2;
            const float yz = y * z2;
            const float zz = z * z2;
            const float wx = w * x2;
            const float wy = w * y2;
            const float wz = w * z2;
            r.data[0] = 1.0f - (yy + zz); r.data[1] = xy + wz;        r.data[2] = xz - wy;
            r.data[4] = xy - wz;          r.data[5] = 1.0f - (xx + zz); r.data[6] = yz + wx;
            r.data[8] = xz + wy;          r.data[9] = yz - wx;         r.data[10] = 1.0f - (xx + yy);
        }
        if (node.scale.size() == 3) {
            s.data[0] = static_cast<float>(node.scale[0]);
            s.data[5] = static_cast<float>(node.scale[1]);
            s.data[10] = static_cast<float>(node.scale[2]);
        }
        local_transform = t * r * s;
    }

    const math::Matrix4x4 global_transform = parent_transform * local_transform;

    if (node.mesh >= 0 && node.mesh < static_cast<int>(model.meshes.size())) {
        const auto& mesh = model.meshes[node.mesh];
        for (const auto& primitive : mesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES || !primitive.attributes.count("POSITION")) {
                continue;
            }

            const auto position_accessor_index = primitive.attributes.at("POSITION");
            const auto positions = read_float_accessor(model, position_accessor_index);
            if (positions.empty() || positions.size() % 3 != 0) {
                continue;
            }

            const auto normals = primitive.attributes.count("NORMAL")
                ? read_float_accessor(model, primitive.attributes.at("NORMAL"))
                : std::vector<float>{};
            const auto uvs = primitive.attributes.count("TEXCOORD_0")
                ? read_float_accessor(model, primitive.attributes.at("TEXCOORD_0"))
                : std::vector<float>{};
            const auto joints = primitive.attributes.count("JOINTS_0")
                ? read_uint_accessor(model, primitive.attributes.at("JOINTS_0"))
                : std::vector<std::uint32_t>{};
            const auto weights = primitive.attributes.count("WEIGHTS_0")
                ? read_float_accessor(model, primitive.attributes.at("WEIGHTS_0"))
                : std::vector<float>{};

            MeshAsset::SubMesh sub_mesh;
            sub_mesh.transform = global_transform;

            if (primitive.material >= 0 && primitive.material < static_cast<int>(model.materials.size())) {
                const auto& mat = model.materials[primitive.material];

                auto it_base = mat.values.find("baseColorFactor");
                if (it_base != mat.values.end()) {
                    sub_mesh.material.base_color_factor = {
                        static_cast<float>(it_base->second.ColorFactor()[0]),
                        static_cast<float>(it_base->second.ColorFactor()[1]),
                        static_cast<float>(it_base->second.ColorFactor()[2])
                    };
                }

                auto it_met = mat.values.find("metallicFactor");
                if (it_met != mat.values.end()) {
                    sub_mesh.material.metallic_factor = static_cast<float>(it_met->second.Factor());
                }

                auto it_rough = mat.values.find("roughnessFactor");
                if (it_rough != mat.values.end()) {
                    sub_mesh.material.roughness_factor = static_cast<float>(it_rough->second.Factor());
                }

                auto it_base_tex = mat.values.find("baseColorTexture");
                if (it_base_tex != mat.values.end()) {
                    sub_mesh.material.base_color_texture_idx = it_base_tex->second.TextureIndex();
                }

                auto it_mr_tex = mat.values.find("metallicRoughnessTexture");
                if (it_mr_tex != mat.values.end()) {
                    sub_mesh.material.metallic_roughness_texture_idx = it_mr_tex->second.TextureIndex();
                }

                auto it_norm = mat.additionalValues.find("normalTexture");
                if (it_norm != mat.additionalValues.end()) {
                    sub_mesh.material.normal_texture_idx = it_norm->second.TextureIndex();
                }
            }

            const std::size_t vertex_count = positions.size() / 3;
            for (std::size_t i = 0; i < vertex_count; ++i) {
                MeshAsset::Vertex vertex;
                vertex.position = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };

                if (normals.size() >= (i + 1) * 3) {
                    vertex.normal = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
                } else {
                    vertex.normal = { 0.0f, 0.0f, 1.0f };
                }

                if (uvs.size() >= (i + 1) * 2) {
                    vertex.uv = { uvs[i * 2 + 0], uvs[i * 2 + 1] };
                } else {
                    vertex.uv = { 0.0f, 0.0f };
                }

                if (joints.size() >= (i + 1) * 4) {
                    for (int j = 0; j < 4; ++j) {
                        vertex.joints[j] = joints[i * 4 + static_cast<std::size_t>(j)];
                    }
                }

                if (weights.size() >= (i + 1) * 4) {
                    for (int j = 0; j < 4; ++j) {
                        vertex.weights[j] = weights[i * 4 + static_cast<std::size_t>(j)];
                    }
                }

                sub_mesh.vertices.push_back(vertex);
            }

            for (const auto& target : primitive.targets) {
                MeshAsset::MorphTarget morph_target;
                if (target.count("POSITION")) {
                    const auto morph_positions = read_float_accessor(model, target.at("POSITION"));
                    for (std::size_t i = 0; i + 2 < morph_positions.size(); i += 3) {
                        morph_target.positions.push_back({ morph_positions[i], morph_positions[i + 1], morph_positions[i + 2] });
                    }
                }
                sub_mesh.morph_targets.push_back(std::move(morph_target));
            }

            if (primitive.indices >= 0) {
                const auto indices = read_index_accessor(model, primitive.indices);
                sub_mesh.indices.insert(sub_mesh.indices.end(), indices.begin(), indices.end());
            } else {
                for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(vertex_count); ++i) {
                    sub_mesh.indices.push_back(i);
                }
            }

            asset->sub_meshes.push_back(std::move(sub_mesh));
        }
    }

    for (int child_idx : node.children) {
        if (child_idx >= 0 && child_idx < static_cast<int>(model.nodes.size())) {
            process_node(model, model.nodes[child_idx], global_transform, asset);
        }
    }
}

std::unique_ptr<MeshAsset> MeshLoader::load_from_gltf(
    const std::filesystem::path& path,
    DiagnosticBag* diagnostics) {

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = false;
    if (path.extension() == ".glb" || path.extension() == ".GLB") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
    } else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    }

    if (!warn.empty() && diagnostics) {
        diagnostics->add_warning("media.mesh.load_warning", warn, path.string());
    }

    if (!ret) {
        if (diagnostics) {
            diagnostics->add_error("media.mesh.load_failed", err, path.string());
        }
        return nullptr;
    }

    auto asset = std::make_unique<MeshAsset>();
    asset->path = path.string();

    for (const auto& image : model.images) {
        TextureData tex;
        tex.width = image.width;
        tex.height = image.height;
        tex.channels = image.component;
        tex.data = image.image;
        asset->textures.push_back(std::move(tex));
    }

    for (const auto& gl_skin : model.skins) {
        MeshAsset::Skin skin;
        skin.name = gl_skin.name;
        for (int joint_idx : gl_skin.joints) {
            MeshAsset::Joint joint;
            joint.index = joint_idx;
            if (joint_idx >= 0 && joint_idx < static_cast<int>(model.nodes.size())) {
                joint.name = model.nodes[joint_idx].name;
            }

            if (gl_skin.inverseBindMatrices >= 0) {
                const auto matrices = read_float_accessor(model, gl_skin.inverseBindMatrices);
                const std::size_t matrix_offset = skin.joints.size() * 16;
                if (matrices.size() >= matrix_offset + 16) {
                    float matrix_data[16];
                    for (std::size_t i = 0; i < 16; ++i) {
                        matrix_data[i] = matrices[matrix_offset + i];
                    }
                    joint.inverse_bind_matrix = math::Matrix4x4(matrix_data);
                }
            }

            skin.joints.push_back(joint);
        }
        asset->skins.push_back(std::move(skin));
    }

    for (const auto& gl_anim : model.animations) {
        MeshAsset::Animation animation;
        animation.name = gl_anim.name;

        for (const auto& gl_chan : gl_anim.channels) {
            if (gl_chan.sampler < 0 || gl_chan.sampler >= static_cast<int>(gl_anim.samplers.size())) {
                continue;
            }

            MeshAsset::AnimationChannel channel;
            channel.node_idx = gl_chan.target_node;

            if (gl_chan.target_path == "translation") {
                channel.path = MeshAsset::AnimationChannel::Path::Translation;
            } else if (gl_chan.target_path == "rotation") {
                channel.path = MeshAsset::AnimationChannel::Path::Rotation;
            } else if (gl_chan.target_path == "scale") {
                channel.path = MeshAsset::AnimationChannel::Path::Scale;
            } else if (gl_chan.target_path == "weights") {
                channel.path = MeshAsset::AnimationChannel::Path::Weights;
            } else {
                continue;
            }

            const auto& sampler = gl_anim.samplers[gl_chan.sampler];
            channel.times = read_float_accessor(model, sampler.input);
            channel.values = read_float_accessor(model, sampler.output);

            if (channel.times.empty() || channel.values.empty()) {
                continue;
            }

            animation.channels.push_back(std::move(channel));
        }

        asset->animations.push_back(std::move(animation));
    }

    const int default_scene = model.defaultScene > -1 ? model.defaultScene : 0;
    if (default_scene >= 0 && default_scene < static_cast<int>(model.scenes.size())) {
        const auto& scene = model.scenes[default_scene];
        for (int node_idx : scene.nodes) {
            if (node_idx >= 0 && node_idx < static_cast<int>(model.nodes.size())) {
                process_node(model, model.nodes[node_idx], math::Matrix4x4::identity(), asset.get());
            }
        }
    }

    if (asset->sub_meshes.empty()) {
        return nullptr;
    }

    return asset;
}

} // namespace tachyon::media
