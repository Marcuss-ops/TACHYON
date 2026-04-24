#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/core/math/matrix4x4.h"
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

        auto it = mesh_cache_.find(instance.mesh_asset_id);
        if (it != mesh_cache_.end()) {
            sub_scene = it->second.scene;
        } else {
            MeshCacheEntry entry;
            entry.scene = rtcNewScene(device_);
            entry.asset = nullptr;

            if (media_manager_) {
                std::filesystem::path mesh_path = media_manager_->get_asset_path(instance.mesh_asset_id);
                if (!mesh_path.empty()) {
                    entry.asset = media_manager_->get_mesh(mesh_path, nullptr);
                    if (entry.asset && !entry.asset->empty()) {
                        entry.submeshes.reserve(entry.asset->sub_meshes.size());
                        for (const auto& sub_mesh : entry.asset->sub_meshes) {
                            if (sub_mesh.vertices.empty() || sub_mesh.indices.empty() || (sub_mesh.indices.size() % 3) != 0) {
                                continue;
                            }

                            MeshCacheEntry::SubMeshCache cached_submesh;
                            cached_submesh.material = sub_mesh.material;
                            cached_submesh.indices = sub_mesh.indices;
                            cached_submesh.vertices.reserve(sub_mesh.vertices.size());

                            for (const auto& vertex : sub_mesh.vertices) {
                                MeshVertex transformed_vertex;
                                transformed_vertex.position = sub_mesh.transform.transform_point(vertex.position);
                                transformed_vertex.normal = transform_normal(sub_mesh.transform, vertex.normal);
                                transformed_vertex.uv = vertex.uv;
                                cached_submesh.vertices.push_back(transformed_vertex);
                            }

                            RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                            const std::size_t vertex_count = cached_submesh.vertices.size();
                            float* v = static_cast<float*>(rtcSetNewGeometryBuffer(
                                geom,
                                RTC_BUFFER_TYPE_VERTEX,
                                0,
                                RTC_FORMAT_FLOAT3,
                                3 * sizeof(float),
                                vertex_count));
                            for (std::size_t i = 0; i < cached_submesh.vertices.size(); ++i) {
                                v[i * 3 + 0] = cached_submesh.vertices[i].position.x;
                                v[i * 3 + 1] = cached_submesh.vertices[i].position.y;
                                v[i * 3 + 2] = cached_submesh.vertices[i].position.z;
                            }

                            const std::size_t triangle_count = cached_submesh.indices.size() / 3;
                            unsigned int* idx = static_cast<unsigned int*>(rtcSetNewGeometryBuffer(
                                geom,
                                RTC_BUFFER_TYPE_INDEX,
                                0,
                                RTC_FORMAT_UINT3,
                                3 * sizeof(unsigned int),
                                triangle_count));
                            for (std::size_t i = 0; i < cached_submesh.indices.size(); ++i) {
                                idx[i] = cached_submesh.indices[i];
                            }

                            rtcCommitGeometry(geom);
                            const unsigned int sub_geom_id = rtcAttachGeometry(entry.scene, geom);
                            entry.geom_id_to_submesh[sub_geom_id] = entry.submeshes.size();
                            entry.submeshes.push_back(std::move(cached_submesh));
                            rtcReleaseGeometry(geom);
                        }
                    }
                }
            }

            rtcCommitScene(entry.scene);
            sub_scene = entry.scene;
            mesh_cache_.emplace(instance.mesh_asset_id, std::move(entry));
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
            instance.mesh_asset_id,
            instance.material,
            instance.world_transform,
            instance.previous_world_transform
        });
        
        rtcReleaseGeometry(inst);
    }
    
    rtcCommitScene(scene_);
}

} // namespace tachyon::renderer3d
