#pragma once

#include "tachyon/core/render/iray_tracer.h"

// These are now handled by IRayTracer inclusion of core/render headers
#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/renderer3d/effects/depth_of_field.h"
#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/renderer3d/lighting/environment_manager.h"
#include "tachyon/media/management/media_manager.h"
#if defined(TACHYON_ENABLE_3D) && __has_include(<embree4/rtcore.h>)
#include <embree4/rtcore.h>
#else
#include "tachyon/third_party/embree_stub.h"
#endif

#ifdef _WIN32
#include <OpenImageDenoise/oidn.hpp>
#endif
#include <memory>
#include <optional>
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

class Modifier3DRegistry;

class RayTracer : public ::tachyon::IRayTracer {
public:
    explicit RayTracer(media::MediaManager* media_manager = nullptr, const Modifier3DRegistry* modifier_registry = nullptr);
    ~RayTracer();

    RayTracer(const RayTracer&) = delete;
    RayTracer& operator=(const RayTracer&) = delete;

    /**
     * @brief Parses an EvaluatedScene3D and builds the Embree BVH structure.
     */
    void build_scene(const EvaluatedScene3D& scene) override;

    /**
     * @brief Performs path tracing to generate multi-pass AOVs.
     * Writes into the provided AOVBuffer natively.
     */
    void render(
        const EvaluatedScene3D& scene,
        AOVBuffer& out_buffer,
        double frame_time_seconds = 0.0,
        double frame_duration_seconds = 0.0) override;
    
    // Internal 3D-specific render call with motion blur helper
    void render_with_motion_blur(
        const EvaluatedScene3D& scene,
        AOVBuffer& out_buffer,
        const MotionBlurRenderer* motion_blur,
        double frame_time_seconds,
        double frame_duration_seconds);
    
    void set_samples_per_pixel(int spp) override { samples_per_pixel_ = spp; }
    void set_denoiser_enabled(bool enabled) override { denoiser_enabled_ = enabled; }

private:
    RTCDevice device_{nullptr};
    RTCScene  scene_{nullptr};

    media::MediaManager* media_manager_{nullptr};
    const media::HDRTextureData* current_env_map_{nullptr};

    struct GeoInstance {
        unsigned int geom_id;
        std::uint32_t object_id;
        std::uint32_t material_id;
        std::string mesh_asset_id;
        std::shared_ptr<const media::MeshAsset> mesh_asset;
        EvaluatedMaterial3D material;
        ::tachyon::math::Matrix4x4 world_transform;
        std::optional<::tachyon::math::Matrix4x4> previous_world_transform;
        std::shared_ptr<std::uint8_t[]> layer_texture;
        int layer_width{0};
        int layer_height{0};
    };
    
    std::vector<GeoInstance> instances_;
    
    struct MeshVertex {
        ::tachyon::math::Vector3 position;
        ::tachyon::math::Vector3 normal;
        ::tachyon::math::Vector2 uv;
    };

    struct MeshCacheEntry {
        RTCScene scene{nullptr};
        std::shared_ptr<const media::MeshAsset> asset;
        struct SubMeshCache {
            std::vector<MeshVertex> vertices;
            std::vector<unsigned int> indices;
            media::MaterialData material;
        };
        std::vector<SubMeshCache> submeshes;
        std::unordered_map<unsigned int, std::size_t> geom_id_to_submesh;
    };
    std::unordered_map<std::string, MeshCacheEntry> mesh_cache_;

    EnvironmentManager environment_manager_;
    
    // Environment members (required by build_scene/render)
    float environment_intensity_{1.0f};
    float environment_rotation_{0.0f};
    std::string environment_map_id_;
    
#ifdef _WIN32
    mutable oidn::DeviceRef oidn_device_;
    mutable oidn::FilterRef oidn_filter_;
#endif

    int samples_per_pixel_{1};
    bool denoiser_enabled_{false};

    ShadingResult trace_ray(
        const math::Vector3& origin,
        const math::Vector3& direction,
        const EvaluatedScene3D& scene,
        std::mt19937& rng,
        int depth,
        float time = 0.0f);

    static constexpr int kMaxBounces = 3;
    static constexpr float kMaxBounceThreshold = 0.001f;

    const std::string& last_error() const override { return m_last_error; }

private:
    std::string m_last_error;
    const Modifier3DRegistry* modifier_registry_{nullptr};

    void cleanup_scene();
    void denoise_aov_buffer(AOVBuffer& buffer) const;
    static void log_embree_error(void* userPtr, RTCError code, const char* str);
};

} // namespace tachyon::renderer3d
