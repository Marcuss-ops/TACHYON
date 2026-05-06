#pragma once

#include <cstdint>
#include <string>

namespace tachyon {

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
};

} // namespace tachyon
