#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/core/math/algebra/matrix4x4.h"
#include "tachyon/media/management/media_manager.h"
#include <filesystem>

namespace tachyon::renderer3d {

namespace {

math::Vector3 transform_normal(const math::Matrix4x4& matrix, const math::Vector3& normal) {
    const math::Matrix4x4 inv = matrix.inverse_affine();
    math::Vector3 transformed {
        inv[0] * normal.x + inv[4] * normal.y + inv[8] * normal.z,
        inv[1] * normal.x + inv[5] * normal.y + inv[9] * normal.z,
        inv[2] * normal.x + inv[6] * normal.y + inv[10] * normal.z
    };
    return transformed.normalized();
}

} // namespace

void RayTracer::build_scene(const EvaluatedScene3D& scene) {
    cleanup_scene();
    if (!device_) return;
    
    scene_ = rtcNewScene(device_);
    
    // Copy environment settings from scene
    environment_intensity_ = scene.environment_intensity;
    environment_rotation_ = scene.environment_rotation;
    environment_map_id_ = scene.environment_map_id;
    
    // Load environment map if available
    current_env_map_ = nullptr;
    if (media_manager_ && !environment_map_id_.empty()) {
        std::filesystem::path env_path = media_manager_->get_asset_path(environment_map_id_);
        if (!env_path.empty()) {
            current_env_map_ = media_manager_->get_hdr_image(env_path, nullptr);
            if (current_env_map_) {
                environment_manager_.update_prefiltered_env(current_env_map_);
            }
        }
    }
    environment_manager_.ensure_brdf_lut();
    
    // Iterate evaluated instances
    for (const auto& instance : scene.instances) {
        RTCScene sub_scene = nullptr;
        bool is_animated = !instance.joint_matrices.empty() || !instance.morph_weights.empty();
        std::string effective_mesh_key = instance.mesh_asset_id;
        
        if (is_animated) {
            effective_mesh_key += "_anim_" + std::to_string(instance.object_id);
        }

        auto it = mesh_cache_.find(effective_mesh_key);
        if (it != mesh_cache_.end()) {
            sub_scene = it->second.scene;
        } else {
            MeshCacheEntry entry;
            entry.scene = rtcNewScene(device_);
            
            // Try to get mesh asset either from instance or from media manager
            std::shared_ptr<const media::MeshAsset> mesh_asset = instance.mesh_asset;
            if (!mesh_asset && media_manager_) {
                std::filesystem::path mesh_path = media_manager_->get_asset_path(instance.mesh_asset_id);
                if (!mesh_path.empty()) {
                    if (const auto* loaded = media_manager_->get_mesh(mesh_path, nullptr)) {
                        mesh_asset = std::shared_ptr<const media::MeshAsset>(loaded, [](const media::MeshAsset*) {});
                    }
                }
            }
            entry.asset = mesh_asset;

            if (mesh_asset) {
                entry.submeshes.reserve(mesh_asset->sub_meshes.size());
                for (const auto& sub_mesh : mesh_asset->sub_meshes) {
                    if (sub_mesh.vertices.empty() || sub_mesh.indices.empty() || (sub_mesh.indices.size() % 3) != 0) {
                        continue;
                    }

                    MeshCacheEntry::SubMeshCache cached_submesh;
                    cached_submesh.material = sub_mesh.material;
                    cached_submesh.indices = sub_mesh.indices;
                    cached_submesh.vertices.reserve(sub_mesh.vertices.size());

                    RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                    const std::size_t vertex_count = sub_mesh.vertices.size();
                    float* v = static_cast<float*>(rtcSetNewGeometryBuffer(
                        geom,
                        RTC_BUFFER_TYPE_VERTEX,
                        0,
                        RTC_FORMAT_FLOAT3,
                        3 * sizeof(float),
                        vertex_count));

                    for (std::size_t i = 0; i < sub_mesh.vertices.size(); ++i) {
                        const auto& v_orig = sub_mesh.vertices[i];
                        math::Vector3 pos = v_orig.position;
                        math::Vector3 normal = v_orig.normal;
                        
                        if (is_animated) {
                            // 1. Apply Morph Targets
                            if (!instance.morph_weights.empty() && !sub_mesh.morph_targets.empty()) {
                                for (std::size_t m = 0; m < std::min(instance.morph_weights.size(), sub_mesh.morph_targets.size()); ++m) {
                                    float weight = instance.morph_weights[m];
                                    if (std::abs(weight) > 1e-4f) {
                                        pos += sub_mesh.morph_targets[m].positions[i] * weight;
                                        if (i < sub_mesh.morph_targets[m].normals.size()) {
                                            normal += sub_mesh.morph_targets[m].normals[i] * weight;
                                        }
                                    }
                                }
                                normal = normal.normalized();
                            }
                            
                            // 2. Apply Skinning
                            if (!instance.joint_matrices.empty()) {
                                math::Vector3 skinned_pos{0,0,0};
                                math::Vector3 skinned_normal{0,0,0};
                                float weight_sum = 0.0f;
                                for (int k = 0; k < 4; ++k) {
                                    float w = v_orig.weights[k];
                                    if (w > 0.0f) {
                                        std::uint32_t joint_idx = v_orig.joints[k];
                                        if (joint_idx < instance.joint_matrices.size()) {
                                            const auto& m = instance.joint_matrices[joint_idx];
                                            skinned_pos += m.transform_point(pos) * w;
                                            skinned_normal += m.transform_vector(normal) * w;
                                            weight_sum += w;
                                        }
                                    }
                                }
                                if (weight_sum > 0.0f) {
                                    pos = skinned_pos * (1.0f / weight_sum);
                                    normal = skinned_normal.normalized();
                                }
                            }
                        }

                        MeshVertex deformed_vertex;
                        deformed_vertex.position = sub_mesh.transform.transform_point(pos);
                        deformed_vertex.normal = transform_normal(sub_mesh.transform, normal);
                        deformed_vertex.uv = v_orig.uv;
                        cached_submesh.vertices.push_back(deformed_vertex);

                        v[i * 3 + 0] = deformed_vertex.position.x;
                        v[i * 3 + 1] = deformed_vertex.position.y;
                        v[i * 3 + 2] = deformed_vertex.position.z;
                    }

                    const std::size_t triangle_count = sub_mesh.indices.size() / 3;
                    unsigned int* idx = static_cast<unsigned int*>(rtcSetNewGeometryBuffer(
                        geom,
                        RTC_BUFFER_TYPE_INDEX,
                        0,
                        RTC_FORMAT_UINT3,
                        3 * sizeof(unsigned int),
                        triangle_count));
                    for (std::size_t i = 0; i < sub_mesh.indices.size(); ++i) {
                        idx[i] = sub_mesh.indices[i];
                    }

                    rtcCommitGeometry(geom);
                    const unsigned int sub_geom_id = rtcAttachGeometry(entry.scene, geom);
                    entry.geom_id_to_submesh[sub_geom_id] = entry.submeshes.size();
                    entry.submeshes.push_back(std::move(cached_submesh));
                    rtcReleaseGeometry(geom);
                }
            }

            rtcCommitScene(entry.scene);
            sub_scene = entry.scene;
            it = mesh_cache_.emplace(effective_mesh_key, std::move(entry)).first;
        }
        
        RTCGeometry inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE);
        rtcSetGeometryInstancedScene(inst, sub_scene);
        
        const auto& m_curr = instance.world_transform;
        if (instance.previous_world_transform.has_value()) {
            const auto& m_prev = *instance.previous_world_transform;
            rtcSetGeometryTimeStepCount(inst, 2);
            float t_prev[12] = { m_prev[0], m_prev[1], m_prev[2], m_prev[4], m_prev[5], m_prev[6], m_prev[8], m_prev[9], m_prev[10], m_prev[12], m_prev[13], m_prev[14] };
            float t_curr[12] = { m_curr[0], m_curr[1], m_curr[2], m_curr[4], m_curr[5], m_curr[6], m_curr[8], m_curr[9], m_curr[10], m_curr[12], m_curr[13], m_curr[14] };
            rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t_prev);
            rtcSetGeometryTransform(inst, 1, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t_curr);
        } else {
            float t[12] = { m_curr[0], m_curr[1], m_curr[2], m_curr[4], m_curr[5], m_curr[6], m_curr[8], m_curr[9], m_curr[10], m_curr[12], m_curr[13], m_curr[14] };
            rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t);
        }
        rtcCommitGeometry(inst);
        
        unsigned int geom_id = rtcAttachGeometry(scene_, inst);
        
        instances_.push_back({
            geom_id,
            instance.object_id,
            instance.material_id,
            effective_mesh_key, // Use the unique key for lookup in shading pass
            instance.material,
            instance.world_transform,
            instance.previous_world_transform
        });
        
        rtcReleaseGeometry(inst);
    }
    
    rtcCommitScene(scene_);
}

} // namespace tachyon::renderer3d
