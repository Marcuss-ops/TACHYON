#include "tachyon/renderer2d/evaluated_composition/rendering/layer_renderer_procedural.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/math/noise.h"
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <random>

namespace tachyon::renderer2d {

namespace {

// --- Math Helpers ---
inline float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// --- 1. Coordinate Warp Stage ---
void apply_warp(float& u, float& v, float t, float strength, float freq, float speed, math::PerlinNoise& noise) {
    if (strength <= 0.0f) return;
    float n1 = noise.noise3d(u * freq, v * freq, t * speed) * strength * 0.1f;
    float n2 = noise.noise3d(v * freq + 0.5f, u * freq + 0.5f, t * speed) * strength * 0.1f;
    u += n1;
    v += n2;
}

// --- 2. Pattern Generators ---
float generate_shape(float u, float v, const std::string& shape, float spacing, float border) {
    float cell_u = std::fmod(u * 1920.0f / spacing, 1.0f);
    float cell_v = std::fmod(v * 1080.0f / spacing, 1.0f);
    float dx = cell_u - 0.5f;
    float dy = cell_v - 0.5f;
    float dist = 0.0f;

    if (shape == "circle") {
        dist = std::sqrt(dx * dx + dy * dy) * 2.0f;
    } else if (shape == "hexagon") {
        float adx = std::abs(dx);
        float ady = std::abs(dy);
        dist = std::max(adx * 0.866025f + ady * 0.5f, ady);
        dist *= 2.0f;
    } else { // default square
        dist = std::max(std::abs(dx), std::abs(dy)) * 2.0f;
    }

    float edge = border / spacing;
    // Widening the range (0.0 to edge * 2.0) creates a soft radial glow instead of a hard circle
    return 1.0f - smoothstep(0.0f, edge * 2.0f, dist);
}

// --- 3. Post-Processing Stage ---
Color apply_post_process(Color c, float u, float v, float t, float grain, float scanlines, float scan_freq, float contrast, float gamma, float saturation) {
    // 1. Scanlines
    if (scanlines > 0.0f) {
        float s = std::sin(v * scan_freq * 6.28318f + t * 5.0f) * 0.5f + 0.5f;
        c.r *= (1.0f - scanlines * s);
        c.g *= (1.0f - scanlines * s);
        c.b *= (1.0f - scanlines * s);
    }

    // 2. Grain (Animated)
    if (grain > 0.0f) {
        float g = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * grain;
        c.r = std::clamp(c.r + g, 0.0f, 1.0f);
        c.g = std::clamp(c.g + g, 0.0f, 1.0f);
        c.b = std::clamp(c.b + g, 0.0f, 1.0f);
    }

    // 3. Contrast & Gamma
    auto adjust = [&](float val) {
        val = std::pow(val, 1.0f / gamma); // Gamma
        val = (val - 0.5f) * contrast + 0.5f; // Contrast
        return std::clamp(val, 0.0f, 1.0f);
    };
    c.r = adjust(c.r);
    c.g = adjust(c.g);
    c.b = adjust(c.b);

    // 4. Saturation
    float luma = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
    c.r = luma * (1.0f - saturation) + c.r * saturation;
    c.g = luma * (1.0f - saturation) + c.g * saturation;
    c.b = luma * (1.0f - saturation) + c.b * saturation;

    return c;
}

} // namespace

void render_procedural_pattern(
    Framebuffer& fb,
    const ProceduralSpec& spec,
    double time_seconds,
    const std::optional<RectI>& target_rect,
    uint32_t comp_width,
    uint32_t comp_height) {
    
    math::PerlinNoise noise(spec.seed);
    std::srand(static_cast<unsigned int>(spec.seed + static_cast<uint64_t>(time_seconds * 1000)));

    // Sample all parameters (Modular logic: everything is available for every pattern)
    float t = static_cast<float>(time_seconds * spec.speed.value.value_or(1.0));
    float freq = static_cast<float>(spec.frequency.value.value_or(1.0));
    float amp = static_cast<float>(spec.amplitude.value.value_or(1.0));
    float scale = static_cast<float>(spec.scale.value.value_or(1.0));
    float warp = static_cast<float>(spec.warp_strength.value.value_or(0.0));
    
    auto to_c = [](const AnimatedColorSpec& s) {
        auto c = s.value.value_or(ColorSpec{0,0,0,255});
        return Color{c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/255.0f};
    };
    Color col_a = to_c(spec.color_a);
    Color col_b = to_c(spec.color_b);
    Color col_c = to_c(spec.color_c);

    const uint32_t width = fb.width();
    const uint32_t height = fb.height();

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            float global_x = static_cast<float>(x + (target_rect ? target_rect->x : 0));
            float global_y = static_cast<float>(y + (target_rect ? target_rect->y : 0));
            float u = global_x / static_cast<float>(comp_width > 0 ? comp_width : 1920);
            float v = global_y / static_cast<float>(comp_height > 0 ? comp_height : 1080);

            // STAGE 1: WARP
            apply_warp(u, v, t, warp, 
                       static_cast<float>(spec.warp_frequency.value.value_or(5.0)), 
                       static_cast<float>(spec.warp_speed.value.value_or(2.0)), noise);

            // STAGE 2: BASE PATTERN
            float value = 0.0f;
            if (spec.kind == "grid") {
                // Add a slow, constant drift for the grid pattern
                float grid_u = u + t * 0.02f; 
                float grid_v = v + t * 0.01f;
                value = generate_shape(grid_u, grid_v, spec.shape, 
                                       static_cast<float>(spec.spacing.value.value_or(50.0)),
                                       static_cast<float>(spec.border_width.value.value_or(1.0)));
            } else if (spec.kind == "aura" || spec.kind == "nebula") {
                float n1 = noise.noise3d(u * freq, v * freq, t * 0.2f);
                float decay = static_cast<float>(spec.octave_decay.value.value_or(0.5));
                float n2 = noise.noise3d(u * freq * 2.0f + n1, v * freq * 2.0f, t * 0.5f) * decay;
                value = (n2 + 1.0f) * 0.5f * amp;
            } else if (spec.kind == "stars") {
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
            if (spec.color_c.value.has_value()) {
                // Three color blend
                if (value < 0.5f) {
                    float t_blend = value * 2.0f;
                    final_color = {col_a.r*(1-t_blend)+col_b.r*t_blend, col_a.g*(1-t_blend)+col_b.g*t_blend, col_a.b*(1-t_blend)+col_b.b*t_blend, 1.0f};
                } else {
                    float t_blend = (value - 0.5f) * 2.0f;
                    final_color = {col_b.r*(1-t_blend)+col_c.r*t_blend, col_b.g*(1-t_blend)+col_c.g*t_blend, col_b.b*(1-t_blend)+col_c.b*t_blend, 1.0f};
                }
            } else {
                final_color = {col_a.r*(1-value)+col_b.r*value, col_a.g*(1-value)+col_b.g*value, col_a.b*(1-value)+col_b.b*value, 1.0f};
            }

            // STAGE 4: POST-PROCESS
            final_color = apply_post_process(final_color, u, v, t,
                                             static_cast<float>(spec.grain_amount.value.value_or(0.0)),
                                             static_cast<float>(spec.scanline_intensity.value.value_or(0.0)),
                                             static_cast<float>(spec.scanline_frequency.value.value_or(100.0)),
                                             static_cast<float>(spec.contrast.value.value_or(1.0)),
                                             static_cast<float>(spec.gamma.value.value_or(1.0)),
                                             static_cast<float>(spec.saturation.value.value_or(1.0)));

            fb.set_pixel(x, y, final_color);
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
