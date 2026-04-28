#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "tachyon/media/loading/mesh_loader.h"
#include <iostream>

namespace tachyon::media {

static void process_node(const tinygltf::Model& model, const tinygltf::Node& node, const math::Matrix4x4& parent_transform, MeshAsset* asset) {
    math::Matrix4x4 local_transform = math::Matrix4x4::identity();

    // 1. Matrix
    if (node.matrix.size() == 16) {
        float m[16];
        for (int i = 0; i < 16; ++i) m[i] = static_cast<float>(node.matrix[i]);
        local_transform = math::Matrix4x4(&m[0]);
    } else {
        // 2. TRS
        math::Matrix4x4 t = math::Matrix4x4::identity();
        math::Matrix4x4 r = math::Matrix4x4::identity();
        math::Matrix4x4 s = math::Matrix4x4::identity();

        if (node.translation.size() == 3) {
            t.data[12] = static_cast<float>(node.translation[0]);
            t.data[13] = static_cast<float>(node.translation[1]);
            t.data[14] = static_cast<float>(node.translation[2]);
        }
        if (node.rotation.size() == 4) {
             // Quaternion to matrix conversion through the local math helpers.
             float x = static_cast<float>(node.rotation[0]);
             float y = static_cast<float>(node.rotation[1]);
             float z = static_cast<float>(node.rotation[2]);
             float w = static_cast<float>(node.rotation[3]);
             float x2 = x + x, y2 = y + y, z2 = z + z;
             float xx = x * x2, xy = x * y2, xz = x * z2;
             float yy = y * y2, yz = y * z2, zz = z * z2;
             float wx = w * x2, wy = w * y2, wz = w * z2;
             r.data[0] = 1.0f - (yy + zz); r.data[1] = xy + wz;        r.data[2] = xz - wy;
             r.data[4] = xy - wz;        r.data[5] = 1.0f - (xx + zz); r.data[6] = yz + wx;
             r.data[8] = xz + wy;        r.data[9] = yz - wx;        r.data[10] = 1.0f - (xx + yy);
        }
        if (node.scale.size() == 3) {
            s.data[0] = static_cast<float>(node.scale[0]);
            s.data[5] = static_cast<float>(node.scale[1]);
            s.data[10] = static_cast<float>(node.scale[2]);
        }
        local_transform = t * r * s;
    }

    math::Matrix4x4 global_transform = parent_transform * local_transform;

    // Process mesh
    if (node.mesh >= 0 && node.mesh < static_cast<int>(model.meshes.size())) {
        const auto& mesh = model.meshes[node.mesh];
        for (const auto& primitive : mesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;

            MeshAsset::SubMesh sub_mesh;
            sub_mesh.transform = global_transform;

            // Material
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
                if (it_met != mat.values.end()) sub_mesh.material.metallic_factor = static_cast<float>(it_met->second.Factor());
                
                auto it_rough = mat.values.find("roughnessFactor");
                if (it_rough != mat.values.end()) sub_mesh.material.roughness_factor = static_cast<float>(it_rough->second.Factor());

                auto it_base_tex = mat.values.find("baseColorTexture");
                if (it_base_tex != mat.values.end()) sub_mesh.material.base_color_texture_idx = it_base_tex->second.TextureIndex();

                auto it_mr_tex = mat.values.find("metallicRoughnessTexture");
                if (it_mr_tex != mat.values.end()) sub_mesh.material.metallic_roughness_texture_idx = it_mr_tex->second.TextureIndex();

                auto it_norm = mat.additionalValues.find("normalTexture");
                if (it_norm != mat.additionalValues.end()) sub_mesh.material.normal_texture_idx = it_norm->second.TextureIndex();
            }

            // Geometry
            const auto& pos_accessor = model.accessors[primitive.attributes.at("POSITION")];
            const auto& pos_buffer_view = model.bufferViews[pos_accessor.bufferView];
            const auto& pos_buffer = model.buffers[pos_buffer_view.buffer];
            const unsigned char* pos_data = &pos_buffer.data[pos_accessor.byteOffset + pos_buffer_view.byteOffset];
            size_t pos_stride = pos_buffer_view.byteStride > 0 ? pos_buffer_view.byteStride : 3 * sizeof(float);

            const unsigned char* norm_data = nullptr;
            size_t norm_stride = 0;
            if (primitive.attributes.count("NORMAL")) {
                const auto& norm_accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& norm_buffer_view = model.bufferViews[norm_accessor.bufferView];
                const auto& norm_buffer = model.buffers[norm_buffer_view.buffer];
                norm_data = &norm_buffer.data[norm_accessor.byteOffset + norm_buffer_view.byteOffset];
                norm_stride = norm_buffer_view.byteStride > 0 ? norm_buffer_view.byteStride : 3 * sizeof(float);
            }

            const unsigned char* uv_data = nullptr;
            size_t uv_stride = 0;
            if (primitive.attributes.count("TEXCOORD_0")) {
                const auto& uv_accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uv_buffer_view = model.bufferViews[uv_accessor.bufferView];
                const auto& uv_buffer = model.buffers[uv_buffer_view.buffer];
                uv_data = &uv_buffer.data[uv_accessor.byteOffset + uv_buffer_view.byteOffset];
                uv_stride = uv_buffer_view.byteStride > 0 ? uv_buffer_view.byteStride : 2 * sizeof(float);
            }

            const unsigned char* joint_data = nullptr;
            size_t joint_stride = 0;
            int joint_type = 0;
            if (primitive.attributes.count("JOINTS_0")) {
                const auto& joint_accessor = model.accessors[primitive.attributes.at("JOINTS_0")];
                const auto& joint_buffer_view = model.bufferViews[joint_accessor.bufferView];
                joint_data = &model.buffers[joint_buffer_view.buffer].data[joint_accessor.byteOffset + joint_buffer_view.byteOffset];
                joint_stride = joint_buffer_view.byteStride > 0 ? joint_buffer_view.byteStride : (joint_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 8 : 4);
                joint_type = joint_accessor.componentType;
            }

            const unsigned char* weight_data = nullptr;
            size_t weight_stride = 0;
            if (primitive.attributes.count("WEIGHTS_0")) {
                const auto& weight_accessor = model.accessors[primitive.attributes.at("WEIGHTS_0")];
                const auto& weight_buffer_view = model.bufferViews[weight_accessor.bufferView];
                weight_data = &model.buffers[weight_buffer_view.buffer].data[weight_accessor.byteOffset + weight_buffer_view.byteOffset];
                weight_stride = weight_buffer_view.byteStride > 0 ? weight_buffer_view.byteStride : 4 * sizeof(float);
            }

            for (std::size_t i = 0; i < pos_accessor.count; ++i) {
                MeshAsset::Vertex v;
                
                const float* p = reinterpret_cast<const float*>(pos_data + i * pos_stride);
                v.position = { p[0], p[1], p[2] };

                if (norm_data) {
                    const float* n = reinterpret_cast<const float*>(norm_data + i * norm_stride);
                    v.normal = { n[0], n[1], n[2] };
                } else {
                    v.normal = { 0, 0, 1 };
                }

                if (uv_data) {
                    const float* u = reinterpret_cast<const float*>(uv_data + i * uv_stride);
                    v.uv = { u[0], u[1] };
                } else {
                    v.uv = { 0, 0 };
                }

                if (joint_data) {
                    if (joint_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        const uint16_t* j = reinterpret_cast<const uint16_t*>(joint_data + i * joint_stride);
                        v.joints[0] = j[0]; v.joints[1] = j[1]; v.joints[2] = j[2]; v.joints[3] = j[3];
                    } else {
                        const uint8_t* j = reinterpret_cast<const uint8_t*>(joint_data + i * joint_stride);
                        v.joints[0] = j[0]; v.joints[1] = j[1]; v.joints[2] = j[2]; v.joints[3] = j[3];
                    }
                }
                if (weight_data) {
                    const float* w = reinterpret_cast<const float*>(weight_data + i * weight_stride);
                    v.weights[0] = w[0]; v.weights[1] = w[1]; v.weights[2] = w[2]; v.weights[3] = w[3];
                }

                sub_mesh.vertices.push_back(v);
            }

            // Morph Targets
            for (const auto& target : primitive.targets) {
                MeshAsset::MorphTarget mt;
                if (target.count("POSITION")) {
                    const auto& acc = model.accessors[target.at("POSITION")];
                    const auto& bv = model.bufferViews[acc.bufferView];
                    const float* data = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset]);
                    for (size_t i = 0; i < acc.count; ++i) mt.positions.push_back({data[i*3+0], data[i*3+1], data[i*3+2]});
                }
                sub_mesh.morph_targets.push_back(std::move(mt));
            }

            if (primitive.indices >= 0) {
                const auto& index_accessor = model.accessors[primitive.indices];
                const auto& index_buffer_view = model.bufferViews[index_accessor.bufferView];
                const auto& index_buffer = model.buffers[index_buffer_view.buffer];

                if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* indices = reinterpret_cast<const uint16_t*>(&index_buffer.data[index_accessor.byteOffset + index_buffer_view.byteOffset]);
                    for (std::size_t i = 0; i < index_accessor.count; ++i) {
                        sub_mesh.indices.push_back(static_cast<unsigned int>(indices[i]));
                    }
                } else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* indices = reinterpret_cast<const uint32_t*>(&index_buffer.data[index_accessor.byteOffset + index_buffer_view.byteOffset]);
                    for (std::size_t i = 0; i < index_accessor.count; ++i) {
                        sub_mesh.indices.push_back(static_cast<unsigned int>(indices[i]));
                    }
                }
            } else {
                // Non-indexed geometry
                for (std::size_t i = 0; i < pos_accessor.count; ++i) {
                    sub_mesh.indices.push_back(static_cast<unsigned int>(i));
                }
            }

            asset->sub_meshes.push_back(std::move(sub_mesh));
        }
    }

    // Process children
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

    // 1. Process Textures
    for (const auto& image : model.images) {
        TextureData tex;
        tex.width = image.width;
        tex.height = image.height;
        tex.channels = image.component;
        tex.data = image.image; // Raw pixels from tinygltf
        asset->textures.push_back(std::move(tex));
    }

    // 2. Process Skins
    for (const auto& gl_skin : model.skins) {
        MeshAsset::Skin skin;
        skin.name = gl_skin.name;
        for (int joint_idx : gl_skin.joints) {
            MeshAsset::Joint j;
            j.index = joint_idx;
            j.name = model.nodes[joint_idx].name;
            // Load inverse bind matrix
            if (gl_skin.inverseBindMatrices >= 0) {
                const auto& acc = model.accessors[gl_skin.inverseBindMatrices];
                const auto& bv = model.bufferViews[acc.bufferView];
                const float* m = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[acc.byteOffset + bv.byteOffset + skin.joints.size() * 16 * sizeof(float)]);
                j.inverse_bind_matrix = math::Matrix4x4(const_cast<float*>(m));
            }
            skin.joints.push_back(j);
        }
        asset->skins.push_back(std::move(skin));
    }

    // 3. Process Animations
    for (const auto& gl_anim : model.animations) {
        MeshAsset::Animation anim;
        anim.name = gl_anim.name;
        for (const auto& gl_chan : gl_anim.channels) {
            MeshAsset::AnimationChannel chan;
            chan.node_idx = gl_chan.target_node;
            if (gl_chan.target_path == "translation") chan.path = MeshAsset::AnimationChannel::Path::Translation;
            else if (gl_chan.target_path == "rotation") chan.path = MeshAsset::AnimationChannel::Path::Rotation;
            else if (gl_chan.target_path == "scale") chan.path = MeshAsset::AnimationChannel::Path::Scale;
            else if (gl_chan.target_path == "weights") chan.path = MeshAsset::AnimationChannel::Path::Weights;
            
            const auto& sampler = gl_anim.samplers[gl_chan.sampler];
            const auto& time_acc = model.accessors[sampler.input];
            const auto& time_bv = model.bufferViews[time_acc.bufferView];
            const float* time_data = reinterpret_cast<const float*>(&model.buffers[time_bv.buffer].data[time_acc.byteOffset + time_bv.byteOffset]);
            for (size_t i = 0; i < time_acc.count; ++i) chan.times.push_back(time_data[i]);
            
            const auto& val_acc = model.accessors[sampler.output];
            const auto& val_bv = model.bufferViews[val_acc.bufferView];
            const float* val_data = reinterpret_cast<const float*>(&model.buffers[val_bv.buffer].data[val_acc.byteOffset + val_bv.byteOffset]);
            int n = (chan.path == MeshAsset::AnimationChannel::Path::Rotation) ? 4 : (chan.path == MeshAsset::AnimationChannel::Path::Weights ? static_cast<int>(model.meshes[0].weights.size()) : 3);
            for (size_t i = 0; i < val_acc.count * n; ++i) chan.values.push_back(val_data[i]);
            
            anim.channels.push_back(std::move(chan));
        }
        asset->animations.push_back(std::move(anim));
    }

    // 4. Process Nodes Hierarchy
    int default_scene = model.defaultScene > -1 ? model.defaultScene : 0;
    if (default_scene >= 0 && default_scene < static_cast<int>(model.scenes.size())) {
        const auto& scene = model.scenes[default_scene];
        for (int node_idx : scene.nodes) {
            if (node_idx >= 0 && node_idx < static_cast<int>(model.nodes.size())) {
                process_node(model, model.nodes[node_idx], math::Matrix4x4::identity(), asset.get());
            }
        }
    }

    if (asset->sub_meshes.empty()) return nullptr;

    return asset;
}

} // namespace tachyon::media
