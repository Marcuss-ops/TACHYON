#include "light_leak_internal.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

namespace {

float smoothstep01(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float radians(float degrees) {
    return degrees * 3.1415926535f / 180.0f;
}

float soft_band(float value, float center, float width, float softness) {
    const float d = std::abs(value - center);
    float x = 1.0f - std::clamp(d / std::max(width, 0.0001f), 0.0f, 1.0f);
    x = smoothstep01(x);
    return std::pow(x, std::max(softness, 0.1f));
}

} // namespace

Color apply_light_leak_style(
    float u,
    float v,
    float t,
    const SurfaceRGBA& input,
    const SurfaceRGBA* to_surface,
    const LightLeakStyle& style
) {
    float mask = 0.0f;
    const bool overlay_mode = (to_surface == nullptr);
    Color base;
    
    // --- CINEMATIC TRANSITION LOGIC ---
    const float color_t = smoothstep01(std::clamp((t - 0.2f) / 0.6f, 0.0f, 1.0f));

    if (overlay_mode) {
        base = sample_uv(input, u, v);
    } else {
        base = Color::lerp(
            sample_uv(input, u, v),
            sample_transition_target(input, to_surface, u, v),
            color_t
        );
    }

    // Optimization: Cache frame-constant values to avoid re-calculating trig/noise for millions of pixels
    static thread_local float last_t = -1.0f;
    static thread_local const char* last_style_id = nullptr;
    static thread_local float cached_cos_a, cached_sin_a, cached_center, cached_flicker;

    if (t != last_t || style.id != last_style_id) {
        const float angle_rad = radians(style.angle_degrees);
        cached_cos_a = std::cos(angle_rad);
        cached_sin_a = std::sin(angle_rad);
        cached_center = -style.offset + t * style.speed;
        const float continuous_flicker = 0.5f + 0.35f * std::sin(t * 41.0f) + 0.15f * std::sin(t * 97.0f);
        cached_flicker = 1.0f + style.flicker_amount * (continuous_flicker - 0.5f);
        last_t = t;
        last_style_id = style.id;
    }

    // Inline the most common shape (Sweep) for performance, fallback to external lookup
    if (style.shape == LightLeakStyle::Shape::Sweep) {
        float proj = u * cached_cos_a + v * cached_sin_a;
        mask = soft_band(proj, cached_center, style.width, style.softness);
    } else {
        mask = evaluate_light_leak_mask(u, v, t, style);
    }
    
    float intensity = style.intensity * mask * cached_flicker * 5.0f;

    // Pulse effect
    if (style.pulse_amount > 0.0f) {
        intensity *= (1.0f + style.pulse_amount * std::sin(t * 12.0f));
    }

    Color leak = Color::lerp(style.color_a, style.color_b, mask);
    
    // Sharper highlight
    leak = Color::lerp(leak, style.highlight, mask * mask * mask);

    // --- DYNAMIC FLUID HUE SHIFT ENGINE ---
    if (style.shape == LightLeakStyle::Shape::CinematicAmber) {
        float modulation = 0.5f + 0.5f * std::sin(t * 2.0f + (u + v) * 1.2f);
        leak.g = std::clamp(leak.g * (0.5f + 0.7f * modulation), 0.18f, 0.65f);
        leak.b *= 0.05f; // Purge cold tones
    }

    // --- PROCEDURAL REMOTION HUE SHIFT ENGINE ---
    // Overrides pre-mixed leak color with dynamic HSL spectral synthesis per uniform hueShift request
    if (style.shape == LightLeakStyle::Shape::ProceduralRemotion) {
        // Ensure continuous wrapping of hue value [0..1]
        float normalized_hue = std::fmod((40.0f + style.angle_degrees) / 360.0f, 1.0f);
        if (normalized_hue < 0.0f) normalized_hue += 1.0f;

        // Replicating the GLSL hsl2rgb(vec3(hue, 0.9, 0.6)) * 1.8;
        const float sat = 0.9f;
        const float light = 0.6f;

        auto hue_to_rgb_channel = [](float p, float q, float t) {
            if (t < 0.0f) t += 1.0f;
            if (t > 1.0f) t -= 1.0f;
            if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
            if (t < 1.0f/2.0f) return q;
            if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
            return p;
        };

        float var_q = (light < 0.5f) ? (light * (1.0f + sat)) : (light + sat - light * sat);
        float var_p = 2.0f * light - var_q;

        leak.r = hue_to_rgb_channel(var_p, var_q, normalized_hue + 1.0f/3.0f) * 1.8f;
        leak.g = hue_to_rgb_channel(var_p, var_q, normalized_hue) * 1.8f;
        leak.b = hue_to_rgb_channel(var_p, var_q, normalized_hue - 1.0f/3.0f) * 1.8f;
    }

    // --- ABSOLUTE SATURATION ENGINE ---
    float hole_factor = smoothstep01(std::clamp(mask / 0.4f, 0.0f, 1.0f));
    base.r *= (1.0f - hole_factor);
    base.g *= (1.0f - hole_factor);
    base.b *= (1.0f - hole_factor);

    // Custom additive blend
    Color result;
    result.r = std::min(base.r + leak.r * intensity, 1.0f);
    result.g = std::min(base.g + leak.g * intensity, 1.0f);
    result.b = std::min(base.b + leak.b * intensity, 1.0f);
    result.a = base.a;

    return result;
}

} // namespace tachyon::renderer2d
