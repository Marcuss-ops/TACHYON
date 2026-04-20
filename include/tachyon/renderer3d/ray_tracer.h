#pragma once

#include "tachyon/core/scene/evaluated_state.h"
#include <embree4/rtcore.h>
#include <memory>
#include <vector>
#include <functional>

namespace tachyon {
namespace renderer3d {

/**
 * CPU Path Tracer using Intel Embree for BVH and ray intersection.
 */
class RayTracer {
public:
    RayTracer();
    ~RayTracer();

    // Prevent copying due to Embree device/scene ownership
    RayTracer(const RayTracer&) = delete;
    RayTracer& operator=(const RayTracer&) = delete;

    /**
     * Builds the Embree BVH from evaluated composition state.
     * This should be called once per frame before rendering.
     */
    void build_scene(const scene::EvaluatedCompositionState& state);

    /**
     * Overload to build scene for a specific subset of layers (for interleaving).
     */
    void build_scene_subset(const scene::EvaluatedCompositionState& state, const std::vector<std::size_t>& layer_indices);

    /**
     * Renders a 3D pass for the current scene.
     * out_rgba: float-based RGBA buffer (HDR).
     * out_depth: optional float buffer for Z-depth.
     */
    void render(const scene::EvaluatedCompositionState& state, float* out_rgba, float* out_depth, int width, int height);
    
    void set_samples_per_pixel(int spp) { samples_per_pixel_ = spp; }

private:
    RTCDevice device_{nullptr};
    RTCScene  scene_{nullptr};

    struct GeoInstance {
        unsigned int geom_id;
        const scene::EvaluatedLayerState* layer_ref;
        const media::MeshAsset* parent_asset{nullptr};
        const media::MeshAsset::SubMesh* sub_mesh{nullptr};
    };
    std::vector<GeoInstance> instances_;
    std::vector<std::unique_ptr<media::MeshAsset>> extruded_assets_;
    const media::HDRTextureData* environment_map_{nullptr};
    float     environment_intensity_{1.0f};
    float     environment_rotation_{0.0f};
    int       samples_per_pixel_{1};

    struct ShadingResult {
        math::Vector3 color;
        float alpha;
        float depth;
    };

    ShadingResult trace_ray(
        const math::Vector3& origin,
        const math::Vector3& direction,
        const scene::EvaluatedCompositionState& state,
        std::mt19937& rng,
        int depth);

    ShadingResult sample_environment(const math::Vector3& direction);

    static constexpr int kMaxBounces = 3;

    void cleanup_scene();
    static void log_embree_error(void* userPtr, RTCError code, const char* str);

    void internal_build_scene(const scene::EvaluatedCompositionState& state, const std::function<bool(std::size_t)>& filter);
};

} // namespace renderer3d
} // namespace tachyon
