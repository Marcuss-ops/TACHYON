#pragma once
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/effects/core/effect_common.h"
#include <algorithm>
#include <atomic>

namespace tachyon::renderer2d {

/**
 * Common utilities for pixel-level transitions.
 */

/**
 * @brief Applies a pixel-level transition function to a surface.
 */
void apply_pixel_transition(
    SurfaceRGBA& result,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    CpuTransitionFn transition_fn,
    float progress,
    float layer_w,
    float layer_h,
    float offset_x,
    float offset_y,
    const std::atomic<bool>* cancel_flag = nullptr);

inline Color sample_uv(const SurfaceRGBA& surface, float u, float v) {
    return sample_texture_bilinear(surface, std::clamp(u, 0.0f, 1.0f), std::clamp(v, 0.0f, 1.0f), Color::white());
}

inline Color sample_transition_target(const SurfaceRGBA& input, const SurfaceRGBA* aux, float u, float v) {
    if (aux != nullptr) {
        return sample_uv(*aux, u, v);
    }
    return sample_uv(input, u, v);
}

inline Color lerp_surface_color(const SurfaceRGBA& a, const SurfaceRGBA* b, float u, float v, float t) {
    const Color ca = sample_uv(a, u, v);
    const Color cb = sample_transition_target(a, b, u, v);
    return Color::lerp(ca, cb, clamp01(t));
}

inline Color screen_over(const Color& base, const Color& overlay, float intensity) {
    const float alpha = clamp01(intensity);
    return {
        1.0f - (1.0f - base.r) * (1.0f - overlay.r * alpha),
        1.0f - (1.0f - base.g) * (1.0f - overlay.g * alpha),
        1.0f - (1.0f - base.b) * (1.0f - overlay.b * alpha),
        std::clamp(base.a + alpha, 0.0f, 1.0f) // Combine base alpha and leak visibility
    };
}

inline Color additive_over(const Color& base, const Color& overlay, float intensity) {
    const float alpha = std::max(0.0f, intensity);
    return {
        std::min(1.0f, base.r + overlay.r * alpha),
        std::min(1.0f, base.g + overlay.g * alpha),
        std::min(1.0f, base.b + overlay.b * alpha),
        std::clamp(base.a + alpha, 0.0f, 1.0f)
    };
}

} // namespace tachyon::renderer2d
