#include "light_leak_internal.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

#ifdef TACHYON_AVX2
#include <immintrin.h>
#endif

namespace tachyon::renderer2d {

namespace {

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
        const float angle_rad = radians(style.angle);
        cached_cos_a = std::cos(angle_rad);
        cached_sin_a = std::sin(angle_rad);
        cached_center = -style.offset + t * style.speed;
        const float continuous_flicker = 0.5f + 0.35f * std::sin(t * 41.0f) + 0.15f * std::sin(t * 97.0f);
        cached_flicker = 1.0f + style.grain * (continuous_flicker - 0.5f);
        last_t = t;
        last_style_id = style.id;
    }

    // Inline the most common shape (Sweep) for performance, fallback to external lookup
    if (style.shape == LightLeakStyle::Shape::Sweep) {
        float proj = u * cached_cos_a + v * cached_sin_a;
        mask = soft_band(proj, cached_center, style.spread, style.softness);
    } else {
        mask = evaluate_light_leak_mask(u, v, t, style);
    }
    
    float intensity = style.intensity * mask * cached_flicker * 5.0f;

    // Pulse effect
    if (style.pulse > 0.0f) {
        intensity *= (1.0f + style.pulse * std::sin(t * 12.0f));
    }

    Color leak = Color::lerp(style.inner_color, style.mid_color, mask);
    
    // Sharper highlight
    leak = Color::lerp(leak, style.outer_color, mask * mask * mask);

    // --- DYNAMIC FLUID HUE SHIFT ENGINE ---
    if (style.shape == LightLeakStyle::Shape::CinematicAmber) {
        float modulation = 0.5f + 0.5f * std::sin(t * 2.0f + (u + v) * 1.2f);
        leak.g = std::clamp(leak.g * (0.5f + 0.7f * modulation), 0.18f, 0.65f);
        leak.b *= 0.05f; // Purge cold tones
    }

    // --- PROCEDURAL REMOTION HUE SHIFT ENGINE ---
    if (style.shape == LightLeakStyle::Shape::ProceduralRemotion) {
        float normalized_hue = std::fmod((40.0f + style.angle) / 360.0f, 1.0f);
        if (normalized_hue < 0.0f) normalized_hue += 1.0f;

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

    Color result;
    result.r = std::min(base.r + leak.r * intensity, 1.0f);
    result.g = std::min(base.g + leak.g * intensity, 1.0f);
    result.b = std::min(base.b + leak.b * intensity, 1.0f);
    result.a = base.a;

    return result;
}

#ifdef TACHYON_AVX2
// AVX2 helper for Sweep mask
void apply_light_leak_sweep_avx2(
    float* __restrict out,
    const float* __restrict from,
    const float* __restrict to,
    uint32_t width,
    uint32_t height,
    float t,
    const LightLeakStyle& style,
    int thread_count
) {
    const float angle_rad = radians(style.angle);
    const float cos_a = std::cos(angle_rad);
    const float sin_a = std::sin(angle_rad);
    const float center = -style.offset + t * style.speed;
    const float continuous_flicker = 0.5f + 0.35f * std::sin(t * 41.0f) + 0.15f * std::sin(t * 97.0f);
    const float flicker = 1.0f + style.grain * (continuous_flicker - 0.5f);
    const float intensity_base = style.intensity * flicker * 5.0f * (style.pulse > 0.0f ? (1.0f + style.pulse * std::sin(t * 12.0f)) : 1.0f);
    
    const float inv_w = 1.0f / static_cast<float>(width);
    const float inv_h = 1.0f / static_cast<float>(height);
    const float color_t = smoothstep01(std::clamp((t - 0.2f) / 0.6f, 0.0f, 1.0f));
    const float inv_color_t = 1.0f - color_t;

    const __m256 v_inv_w = _mm256_set1_ps(inv_w);
    const __m256 v_inv_h = _mm256_set1_ps(inv_h);
    const __m256 v_cos_a = _mm256_set1_ps(cos_a);
    const __m256 v_sin_a = _mm256_set1_ps(sin_a);
    const __m256 v_center = _mm256_set1_ps(center);
    const __m256 v_inv_spread = _mm256_set1_ps(1.0f / std::max(style.spread, 0.0001f));
    const __m256 v_intensity = _mm256_set1_ps(intensity_base);
    const __m256 v_color_t = _mm256_set1_ps(color_t);
    const __m256 v_inv_color_t = _mm256_set1_ps(inv_color_t);
    const __m256 v_one = _mm256_set1_ps(1.0f);
    const __m256 v_zero = _mm256_setzero_ps();
    const __m256 v_three = _mm256_set1_ps(3.0f);
    const __m256 v_two = _mm256_set1_ps(2.0f);

    const __m256 v_inner_r = _mm256_set1_ps(style.inner_color.r);
    const __m256 v_inner_g = _mm256_set1_ps(style.inner_color.g);
    const __m256 v_inner_b = _mm256_set1_ps(style.inner_color.b);
    const __m256 v_mid_r = _mm256_set1_ps(style.mid_color.r);
    const __m256 v_mid_g = _mm256_set1_ps(style.mid_color.g);
    const __m256 v_mid_b = _mm256_set1_ps(style.mid_color.b);
    const __m256 v_outer_r = _mm256_set1_ps(style.outer_color.r);
    const __m256 v_outer_g = _mm256_set1_ps(style.outer_color.g);
    const __m256 v_outer_b = _mm256_set1_ps(style.outer_color.b);

    #pragma omp parallel for num_threads(thread_count) schedule(static)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        float v_val = (static_cast<float>(y) + 0.5f) * inv_h;
        __m256 vv = _mm256_set1_ps(v_val);
        
        for (int x = 0; x < static_cast<int>(width); x += 8) {
            __m256 vx = _mm256_set_ps(
                static_cast<float>(x + 7) + 0.5f, static_cast<float>(x + 6) + 0.5f,
                static_cast<float>(x + 5) + 0.5f, static_cast<float>(x + 4) + 0.5f,
                static_cast<float>(x + 3) + 0.5f, static_cast<float>(x + 2) + 0.5f,
                static_cast<float>(x + 1) + 0.5f, static_cast<float>(x + 0) + 0.5f
            );
            __m256 vu = _mm256_mul_ps(vx, v_inv_w);

            __m256 v_proj = _mm256_add_ps(_mm256_mul_ps(vu, v_cos_a), _mm256_mul_ps(vv, v_sin_a));
            __m256 v_dist = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), _mm256_sub_ps(v_proj, v_center));
            __m256 v_x = _mm256_sub_ps(v_one, _mm256_max_ps(v_zero, _mm256_min_ps(v_one, _mm256_mul_ps(v_dist, v_inv_spread))));
            __m256 v_mask = _mm256_mul_ps(_mm256_mul_ps(v_x, v_x), _mm256_sub_ps(v_three, _mm256_mul_ps(v_two, v_x)));
            
            __m256 v_leak_r = _mm256_add_ps(v_inner_r, _mm256_mul_ps(_mm256_sub_ps(v_mid_r, v_inner_r), v_mask));
            __m256 v_leak_g = _mm256_add_ps(v_inner_g, _mm256_mul_ps(_mm256_sub_ps(v_mid_g, v_inner_g), v_mask));
            __m256 v_leak_b = _mm256_add_ps(v_inner_b, _mm256_mul_ps(_mm256_sub_ps(v_mid_b, v_inner_b), v_mask));

            __m256 v_mask3 = _mm256_mul_ps(v_mask, _mm256_mul_ps(v_mask, v_mask));
            v_leak_r = _mm256_add_ps(v_leak_r, _mm256_mul_ps(_mm256_sub_ps(v_outer_r, v_leak_r), v_mask3));
            v_leak_g = _mm256_add_ps(v_leak_g, _mm256_mul_ps(_mm256_sub_ps(v_outer_g, v_leak_g), v_mask3));
            v_leak_b = _mm256_add_ps(v_leak_b, _mm256_mul_ps(_mm256_sub_ps(v_outer_b, v_leak_b), v_mask3));

            __m256 v_final_intensity = _mm256_mul_ps(v_intensity, v_mask);
            __m256 v_hole = _mm256_max_ps(v_zero, _mm256_min_ps(v_one, _mm256_mul_ps(v_mask, _mm256_set1_ps(2.5f))));
            v_hole = _mm256_mul_ps(_mm256_mul_ps(v_hole, v_hole), _mm256_sub_ps(v_three, _mm256_mul_ps(v_two, v_hole)));
            __m256 v_inv_hole = _mm256_sub_ps(v_one, v_hole);

            for (int i = 0; i < 8; ++i) {
                if (x + i >= static_cast<int>(width)) break;
                size_t offset = (static_cast<size_t>(y) * width + x + i) * 4;
                
                float leak_r = reinterpret_cast<float*>(&v_leak_r)[i];
                float leak_g = reinterpret_cast<float*>(&v_leak_g)[i];
                float leak_b = reinterpret_cast<float*>(&v_leak_b)[i];
                float intensity = reinterpret_cast<float*>(&v_final_intensity)[i];
                float inv_hole = reinterpret_cast<float*>(&v_inv_hole)[i];

                float base_r, base_g, base_b, base_a;
                if (to) {
                    base_r = from[offset+0] * inv_color_t + to[offset+0] * color_t;
                    base_g = from[offset+1] * inv_color_t + to[offset+1] * color_t;
                    base_b = from[offset+2] * inv_color_t + to[offset+2] * color_t;
                    base_a = from[offset+3] * inv_color_t + to[offset+3] * color_t;
                } else {
                    base_r = from[offset+0];
                    base_g = from[offset+1];
                    base_b = from[offset+2];
                    base_a = from[offset+3];
                }

                base_r *= inv_hole;
                base_g *= inv_hole;
                base_b *= inv_hole;

                out[offset+0] = std::min(base_r + leak_r * intensity, 1.0f);
                out[offset+1] = std::min(base_g + leak_g * intensity, 1.0f);
                out[offset+2] = std::min(base_b + leak_b * intensity, 1.0f);
                out[offset+3] = base_a;
            }
        }
    }
}
#endif

void apply_light_leak_batch(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    const LightLeakStyle& style,
    int thread_count
) {
    const uint32_t width = output.width();
    const uint32_t height = output.height();
    float* out_data = output.mutable_pixels().data();
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;

#ifdef TACHYON_AVX2
    if (style.shape == LightLeakStyle::Shape::Sweep && width % 8 == 0) {
        apply_light_leak_sweep_avx2(out_data, from_data, to_data, width, height, progress, style, thread_count);
        return;
    }
#endif

    const float inv_w = 1.0f / static_cast<float>(width);
    const float inv_h = 1.0f / static_cast<float>(height);

    #pragma omp parallel for num_threads(thread_count) schedule(dynamic, 16)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        float v = (static_cast<float>(y) + 0.5f) * inv_h;
        for (int x = 0; x < static_cast<int>(width); ++x) {
            float u = (static_cast<float>(x) + 0.5f) * inv_w;
            Color res = apply_light_leak_style(u, v, progress, from, to, style);
            size_t offset = (static_cast<size_t>(y) * width + x) * 4;
            out_data[offset + 0] = res.r;
            out_data[offset + 1] = res.g;
            out_data[offset + 2] = res.b;
            out_data[offset + 3] = res.a;
        }
    }
}

} // namespace tachyon::renderer2d
