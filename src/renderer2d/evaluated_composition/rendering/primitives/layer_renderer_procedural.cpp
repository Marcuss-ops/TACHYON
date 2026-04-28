#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_procedural.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/math/utils/noise.h"
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <random>
#include <omp.h>

namespace tachyon::renderer2d {

namespace {

// --- Math Helpers ---
inline float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// --- Shape Generator (used by grid pattern) ---
float generate_shape(float u, float v, const std::string& shape, float spacing, float border) {
    float cell_u = std::fmod(u * 1920.0f / spacing, 1.0f);
    float cell_v = std::fmod(v * 1080.0f / spacing, 1.0f);
    float dx = cell_u - 0.5f;
    float dy = cell_v - 0.5f;
    float dist = 0.0f;

    if (shape == "circle") {
        dist = std::sqrt(dx * dx + dy * dy) * 2.0f;
    } else { // default square (hexagon/triangle removed per request)
        dist = std::max(std::abs(dx), std::abs(dy)) * 2.0f;
    }

    float edge = border / spacing;
    return 1.0f - smoothstep(0.0f, edge * 2.0f, dist);
}

} // namespace

void render_procedural_pattern(
    Framebuffer& fb,
    const ProceduralSpec& spec,
    double time_seconds,
    const std::optional<RectI>& target_rect,
    uint32_t comp_width,
    uint32_t comp_height) {
    
    // Pre-compute all invariant parameters outside the loop
    const float t = static_cast<float>(time_seconds * spec.speed.value.value_or(1.0));
    const float freq = static_cast<float>(spec.frequency.value.value_or(1.0));
    const float amp = static_cast<float>(spec.amplitude.value.value_or(1.0));
    const float scale = static_cast<float>(spec.scale.value.value_or(1.0));
    const float warp = static_cast<float>(spec.warp_strength.value.value_or(0.0));
    const float warp_freq = static_cast<float>(spec.warp_frequency.value.value_or(5.0));
    const float warp_speed = static_cast<float>(spec.warp_speed.value.value_or(2.0));
    const float octave_decay = static_cast<float>(spec.octave_decay.value.value_or(0.5));
    const float spacing = static_cast<float>(spec.spacing.value.value_or(50.0));
    const float border = static_cast<float>(spec.border_width.value.value_or(1.0));
    const std::string& shape = spec.shape;
    
    // Pre-compute colors
    auto to_c = [](const AnimatedColorSpec& s) {
        auto c = s.value.value_or(ColorSpec{0,0,0,255});
        return Color{c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/255.0f};
    };
    const Color col_a = to_c(spec.color_a);
    const Color col_b = to_c(spec.color_b);
    const Color col_c = to_c(spec.color_c);
    const bool has_color_c = spec.color_c.value.has_value();
    
    // Pre-compute post-processing params
    const float grain = static_cast<float>(spec.grain_amount.value.value_or(0.0));
    const float scanlines = static_cast<float>(spec.scanline_intensity.value.value_or(0.0));
    const float scan_freq = static_cast<float>(spec.scanline_frequency.value.value_or(100.0));
    const float contrast = static_cast<float>(spec.contrast.value.value_or(1.0));
    const float gamma = static_cast<float>(spec.gamma.value.value_or(1.0));
    const float saturation = static_cast<float>(spec.saturation.value.value_or(1.0));
    const float inv_gamma = 1.0f / gamma;
    
    // Pattern type detection
    const bool is_aura = (spec.kind == "aura" || spec.kind == "nebula");
    const bool is_grid = (spec.kind == "grid");
    const bool is_stars = (spec.kind == "stars");
    const bool do_warp = (warp > 0.0f);
    
    const uint32_t width = fb.width();
    const uint32_t height = fb.height();
    const int offset_x = target_rect ? target_rect->x : 0;
    const int offset_y = target_rect ? target_rect->y : 0;
    const float inv_comp_w = 1.0f / static_cast<float>(comp_width > 0 ? comp_width : 1920);
    const float inv_comp_h = 1.0f / static_cast<float>(comp_height > 0 ? comp_height : 1080);
    
    // Get mutable pixels for direct access
    auto& pixels = fb.mutable_pixels();
    const uint32_t pixel_stride = 4;
    
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        math::PerlinNoise noise(spec.seed + static_cast<uint64_t>(y));
        // Use thread-local random state (rand_r not available on Windows)
       thread_local unsigned int thread_rand_seed = static_cast<unsigned int>(spec.seed);
        thread_rand_seed = static_cast<unsigned int>(spec.seed + static_cast<uint64_t>(time_seconds * 1000) + y);
        
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const int pixel_idx = (y * width + x) * pixel_stride;
            
            float global_x = static_cast<float>(x + offset_x);
            float global_y = static_cast<float>(y + offset_y);
            float u = global_x * inv_comp_w;
            float v = global_y * inv_comp_h;
            
            // STAGE 1: WARP
            if (do_warp) {
                float n1 = noise.noise3d(u * warp_freq, v * warp_freq, t * warp_speed) * warp * 0.1f;
                float n2 = noise.noise3d(v * warp_freq + 0.5f, u * warp_freq + 0.5f, t * warp_speed) * warp * 0.1f;
                u += n1;
                v += n2;
            }
            
            // STAGE 2: BASE PATTERN
            float value = 0.0f;
            if (is_aura) {
                float n1 = noise.noise3d(u * freq, v * freq, t * 0.2f);
                float n2 = noise.noise3d(u * freq * 2.0f + n1, v * freq * 2.0f, t * 0.5f) * octave_decay;
                value = (n2 + 1.0f) * 0.5f * amp;
            } else if (is_grid) {
                float rad = static_cast<float>(spec.angle.value.value_or(0.0) * 3.14159265358979323846 / 180.0);
                float grid_u = u + t * std::cos(rad) * 0.5f;
                float grid_v = v + t * std::sin(rad) * 0.5f;
                value = generate_shape(grid_u, grid_v, shape, spacing, border);
            } else if (is_stars) {
                float n = noise.noise3d(u * freq * 100.0f, v * freq * 100.0f, 0.0f);
                if (n > 0.8f) {
                    float twinkle = std::sin(t * 3.0f + n * 10.0f) * 0.5f + 0.5f;
                    value = ((n - 0.8f) / 0.2f) * twinkle * amp;
                }
            } else {
                value = (noise.noise3d(u * freq * scale, v * freq * scale, t) + 1.0f) * 0.5f * amp;
            }
            
            // STAGE 3: COLOR BLEND
            Color final_color;
            if (has_color_c) {
                if (value < 0.5f) {
                    float t_blend = value * 2.0f;
                    final_color.r = col_a.r*(1-t_blend)+col_b.r*t_blend;
                    final_color.g = col_a.g*(1-t_blend)+col_b.g*t_blend;
                    final_color.b = col_a.b*(1-t_blend)+col_b.b*t_blend;
                    final_color.a = 1.0f;
                } else {
                    float t_blend = (value - 0.5f) * 2.0f;
                    final_color.r = col_b.r*(1-t_blend)+col_c.r*t_blend;
                    final_color.g = col_b.g*(1-t_blend)+col_c.g*t_blend;
                    final_color.b = col_b.b*(1-t_blend)+col_c.b*t_blend;
                    final_color.a = 1.0f;
                }
            } else {
                final_color.r = col_a.r*(1-value)+col_b.r*value;
                final_color.g = col_a.g*(1-value)+col_b.g*value;
                final_color.b = col_a.b*(1-value)+col_b.b*value;
                final_color.a = 1.0f;
            }
            
            // STAGE 4: POST-PROCESS
            if (scanlines > 0.0f) {
                float s = std::sin(v * scan_freq * 6.28318f + t * 5.0f) * 0.5f + 0.5f;
                final_color.r *= (1.0f - scanlines * s);
                final_color.g *= (1.0f - scanlines * s);
                final_color.b *= (1.0f - scanlines * s);
            }
            
            if (grain > 0.0f) {
                // Simple LCG random for thread-safe grain
                thread_rand_seed = thread_rand_seed * 1664525U + 1013904223U;
                float g = (static_cast<float>(thread_rand_seed & 0xFFFFFF) / static_cast<float>(0xFFFFFF) - 0.5f) * grain;
                final_color.r = std::clamp(final_color.r + g, 0.0f, 1.0f);
                final_color.g = std::clamp(final_color.g + g, 0.0f, 1.0f);
                final_color.b = std::clamp(final_color.b + g, 0.0f, 1.0f);
            }
            
            // Contrast & Gamma
            final_color.r = std::clamp(std::pow(final_color.r, inv_gamma) * contrast + (0.5f - 0.5f * contrast), 0.0f, 1.0f);
            final_color.g = std::clamp(std::pow(final_color.g, inv_gamma) * contrast + (0.5f - 0.5f * contrast), 0.0f, 1.0f);
            final_color.b = std::clamp(std::pow(final_color.b, inv_gamma) * contrast + (0.5f - 0.5f * contrast), 0.0f, 1.0f);
            
            // Saturation
            float luma = final_color.r * 0.299f + final_color.g * 0.587f + final_color.b * 0.114f;
            final_color.r = luma * (1.0f - saturation) + final_color.r * saturation;
            final_color.g = luma * (1.0f - saturation) + final_color.g * saturation;
            final_color.b = luma * (1.0f - saturation) + final_color.b * saturation;
            
            // Write directly to pixel buffer
            pixels[pixel_idx + 0] = final_color.r;
            pixels[pixel_idx + 1] = final_color.g;
            pixels[pixel_idx + 2] = final_color.b;
            pixels[pixel_idx + 3] = final_color.a;
        }
    }
}

void render_procedural_layer(
    Framebuffer& fb,
    const tachyon::scene::EvaluatedLayerState& evaluated,
    double time_seconds,
    const std::optional<RectI>& target_rect) {
    
    if (!evaluated.procedural.has_value()) return;
    uint32_t comp_w = static_cast<uint32_t>(evaluated.width);
    uint32_t comp_h = static_cast<uint32_t>(evaluated.height);
    render_procedural_pattern(fb, *evaluated.procedural, time_seconds, target_rect, comp_w, comp_h);
}

} // namespace tachyon::renderer2d

