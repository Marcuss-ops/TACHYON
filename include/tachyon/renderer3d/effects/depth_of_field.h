#pragma once

#include "tachyon/renderer3d/core/aov_buffer.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/runtime/execution/planning/quality_policy.h"

#include <vector>
#include <cstdint>
#include <cmath>

namespace tachyon::renderer3d {

/**
 * @brief Depth of Field post-pass using Circle of Confusion from Z-depth AOV.
 * 
 * This is a separable Gaussian-like blur applied after the beauty pass.
 * The blur radius per pixel is computed from the depth_z AOV and camera parameters.
 */
enum class BokehQualityTier {
    Draft,    // Gaussian-ish separable (Fast)
    Preview,  // Circular convolution (Production)
    Final     // Shaped bokeh with highlights (High-end)
};

struct DepthOfFieldConfig {
    bool enabled{false};
    float focal_distance{10.0f};    // World units where objects are in focus
    float aperture_fstop{2.8f};     // f-stop (lower = more blur)
    float focal_length_mm{35.0f};   // Physical focal length
    float sensor_width_mm{36.0f};   // Full frame sensor
    float max_blur_radius_px{20.0f}; // Max blur radius in pixels
    int blur_samples{16};           // Samples per pixel for bokeh blur
    int diaphragm_blades{0};        // 0=circle, 3+=polygonal shape
    float bokeh_rotation_deg{0.0f}; // Rotation of the polygonal shape
    BokehQualityTier quality{BokehQualityTier::Preview};

    /**
     * @brief Create a DoF config from a render QualityPolicy.
     *
     * Maps technical quality parameters (max blur radius, sample count) from
     * the tier. Artistic parameters (focal_distance, aperture) use defaults
     * unless overridden by the caller.
     */
    static DepthOfFieldConfig from_quality_policy(
        const ::tachyon::QualityPolicy& policy,
        float focal_distance = 10.0f,
        float aperture_fstop = 2.8f,
        float focal_length_mm = 35.0f,
        float sensor_width_mm = 36.0f)
    {
        DepthOfFieldConfig config;
        config.enabled = policy.dof_sample_count > 1;
        config.focal_distance = focal_distance;
        config.aperture_fstop = aperture_fstop;
        config.focal_length_mm = focal_length_mm;
        config.sensor_width_mm = sensor_width_mm;
        config.max_blur_radius_px = policy.max_coc_radius_px;
        config.blur_samples = policy.dof_sample_count;
        if (policy.dof_sample_count <= 4) {
            config.quality = BokehQualityTier::Draft;
        } else if (policy.dof_sample_count <= 16) {
            config.quality = BokehQualityTier::Preview;
        } else {
            config.quality = BokehQualityTier::Final;
        }
        return config;
    }
};

class DepthOfFieldPostPass {
public:
    DepthOfFieldPostPass() = default;
    explicit DepthOfFieldPostPass(const DepthOfFieldConfig& config) : config_(config) {}

    void set_config(const DepthOfFieldConfig& config) { config_ = config; }
    const DepthOfFieldConfig& get_config() const { return config_; }
    bool is_enabled() const { return config_.enabled; }

    /**
     * @brief Apply DoF blur to the beauty pass using depth_z AOV.
     * 
     * @param aovs The AOV buffer containing beauty_rgba and depth_z.
     * @param camera_position World position of the camera.
     * @param camera_forward Forward direction of the camera (normalized).
     * 
     * Modifies aovs.beauty_rgba in-place.
     */
    void apply(AOVBuffer& aovs,
               const math::Vector3& camera_position,
               const math::Vector3& camera_forward) const;

private:
    DepthOfFieldConfig config_;

    float compute_coc_radius(float depth, float focal_distance, float aperture, float focal_length, float sensor_width) const;
    void separable_blur(std::vector<float>& rgba, const std::vector<float>& depth, std::uint32_t width, std::uint32_t height, float max_radius) const;
    void circular_convolution_pass(std::vector<float>& rgba, const std::vector<float>& depth, std::uint32_t width, std::uint32_t height) const;
    void cinematic_bokeh_pass(std::vector<float>& rgba, const std::vector<float>& depth, std::uint32_t width, std::uint32_t height) const;
};

} // namespace tachyon::renderer3d
