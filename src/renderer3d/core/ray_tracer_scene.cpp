#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/core/math/matrix4x4.h"

namespace tachyon::renderer3d {

void RayTracer::cleanup_scene() {
    if (scene_) { rtcReleaseScene(scene_); scene_ = nullptr; }
    instances_.clear();
}

void RayTracer::build_scene(const EvaluatedScene3D& scene) {
    cleanup_scene();
    if (!device_) return;
    
    scene_ = rtcNewScene(device_);
    
    // Setup environment
    environment_intensity_ = scene.environment_intensity;
    // Note: HDR texture lookup would happen here based on scene.environment_map_id
    // environment_manager_.update_prefiltered_env(...);
    environment_manager_.ensure_brdf_lut();

    // Iterate evaluated instances instead of 2D composition layers
    for (const auto& instance : scene.instances) {
        RTCScene sub_scene = nullptr;
        
        auto it = mesh_cache_.find(instance.mesh_asset_id);
        if (it != mesh_cache_.end()) {
            sub_scene = it->second.scene;
        } else {
            // Note: In a complete implementation, this would pull from the AssetManager
            // For this milestone, we construct a generic placeholder quad to satisfy Embree
            sub_scene = rtcNewScene(device_); 
            RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
            float* v = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), 4);
            v[0] = -1.0f; v[1] = -1.0f; v[2] = 0.0f; 
            v[3] =  1.0f; v[4] = -1.0f; v[5] = 0.0f; 
            v[6] =  1.0f; v[7] =  1.0f; v[8] = 0.0f; 
            v[9] = -1.0f; v[10] = 1.0f; v[11] = 0.0f;
            
            unsigned int* idx = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), 2);
            idx[0] = 0; idx[1] = 1; idx[2] = 2; 
            idx[3] = 0; idx[4] = 2; idx[5] = 3;
            
            rtcCommitGeometry(geom); 
            rtcAttachGeometry(sub_scene, geom); 
            rtcReleaseGeometry(geom); 
            rtcCommitScene(sub_scene); 
            mesh_cache_[instance.mesh_asset_id] = {sub_scene};
        }

        RTCGeometry inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE); 
        rtcSetGeometryInstancedScene(inst, sub_scene);
        
        const auto& m = instance.world_transform; 
        float t[12] = { m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14] };
        rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t); 
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
