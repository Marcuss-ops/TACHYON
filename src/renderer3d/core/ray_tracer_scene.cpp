#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/media/loading/mesh_loader.h"
#include "tachyon/media/management/media_manager.h"
#include <filesystem>

namespace tachyon::renderer3d {

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
            sub_scene = rtcNewScene(device_);
            // Load real mesh from asset
            if (media_manager_) {
                std::filesystem::path mesh_path = media_manager_->get_asset_path(instance.mesh_asset_id);
                if (!mesh_path.empty()) {
                    auto mesh_asset = media::MeshLoader::load_from_gltf(mesh_path, nullptr);
                    if (mesh_asset && !mesh_asset->empty()) {
                        for (const auto& sub_mesh : mesh_asset->sub_meshes) {
                            if (sub_mesh.vertices.empty() || sub_mesh.indices.empty() || (sub_mesh.indices.size() % 3) != 0) continue;
                            RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                            // Set vertex buffer: itemCount = number of vertices
                            const std::size_t vertex_count = sub_mesh.vertices.size();
                            float* v = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), vertex_count);
                            for (size_t i = 0; i < sub_mesh.vertices.size(); ++i) {
                                v[i*3 + 0] = sub_mesh.vertices[i].position.x;
                                v[i*3 + 1] = sub_mesh.vertices[i].position.y;
                                v[i*3 + 2] = sub_mesh.vertices[i].position.z;
                            }
                            // Set index buffer: itemCount = number of triangles.
                            const std::size_t triangle_count = sub_mesh.indices.size() / 3;
                            unsigned int* idx = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), triangle_count);
                            for (size_t i = 0; i < sub_mesh.indices.size(); ++i) {
                                idx[i] = sub_mesh.indices[i];
                            }
                            rtcCommitGeometry(geom);
                            rtcAttachGeometry(sub_scene, geom);
                            rtcReleaseGeometry(geom);
                        }
                    }
                }
            }
            rtcCommitScene(sub_scene);
            mesh_cache_[instance.mesh_asset_id] = {sub_scene};
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
            instance.material,
            instance.world_transform,
            instance.previous_world_transform
        });
        
        rtcReleaseGeometry(inst);
    }
    
    rtcCommitScene(scene_);
}

} // namespace tachyon::renderer3d
