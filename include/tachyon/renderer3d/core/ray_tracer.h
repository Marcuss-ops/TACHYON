#pragma once

#include "tachyon/renderer3d/core/evaluated_scene_3d.h"
#include "tachyon/renderer3d/core/aov_buffer.h"
#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/renderer3d/effects/depth_of_field.h"
#include "tachyon/renderer3d/lighting/environment_manager.h"
#include <embree4/rtcore.h>
#include <OpenImageDenoise/oidn.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <random>
#include <string>

namespace tachyon::renderer3d {

struct ShadingResult {
    ::tachyon::math::Vector3 color;
    float alpha;
    float depth;
    ::tachyon::math::Vector3 normal;
    ::tachyon::math::Vector3 albedo;
    ::tachyon::math::Vector2 motion_vector;
    std::uint32_t object_id;
    std::uint32_t material_id;
};

class RayTracer {
public:
    RayTracer();
    ~RayTracer();

    RayTracer(const RayTracer&) = delete;
    RayTracer& operator=(const RayTracer&) = delete;

    /**
     * @brief Parses an EvaluatedScene3D and builds the Embree BVH structure.
     */
    void build_scene(const EvaluatedScene3D& scene);

    /**
     * @brief Performs path tracing to generate multi-pass AOVs.
     * Writes into the provided AOVBuffer natively.
     */
    void render(
        const EvaluatedScene3D& scene,
        AOVBuffer& out_buffer,
        const MotionBlurRenderer* motion_blur = nullptr,
        double frame_time_seconds = 0.0,
        double frame_duration_seconds = 0.0);
    
    void set_samples_per_pixel(int spp) { samples_per_pixel_ = spp; }

private:
    RTCDevice device_{nullptr};
    RTCScene  scene_{nullptr};

    struct GeoInstance {
        unsigned int geom_id;
        std::uint32_t object_id;
        std::uint32_t material_id;
        EvaluatedMaterial material;
        ::tachyon::math::Matrix4x4 world_transform;
        std::optional<::tachyon::math::Matrix4x4> previous_world_transform;
    };
    
    std::vector<GeoInstance> instances_;
    
    struct MeshCacheEntry {
        RTCScene scene;
    };
    std::unordered_map<std::string, MeshCacheEntry> mesh_cache_;

    EnvironmentManager environment_manager_;
    
    oidn::DeviceRef oidn_device_;
    oidn::FilterRef oidn_filter_;

    int samples_per_pixel_{1};

    ShadingResult trace_ray(
        const math::Vector3& origin,
        const math::Vector3& direction,
        const EvaluatedScene3D& scene,
        std::mt19937& rng,
        int depth,
        float time = 0.0f);

    static constexpr int kMaxBounces = 3;

    const std::string& last_error() const { return m_last_error; }

private:
    std::string m_last_error;

    void cleanup_scene();
    static void log_embree_error(void* userPtr, RTCError code, const char* str);
};

} // namespace tachyon::renderer3d
