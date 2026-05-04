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
float generate_shape(float u, float v, const std::string& shape, float spacing, float border, float comp_width, float comp_height) {
    const float safe_width = comp_width > 0.0f ? comp_width : 1920.0f;
    const float safe_height = comp_height > 0.0f ? comp_height : 1080.0f;
    float cell_u = std::fmod(u * safe_width / spacing, 1.0f);
    float cell_v = std::fmod(v * safe_height / spacing, 1.0f);
    if (cell_u < 0.0f) cell_u += 1.0f;
    if (cell_v < 0.0f) cell_v += 1.0f;
    const float edge_u = std::min(cell_u, 1.0f - cell_u) * spacing;
    const float edge_v = std::min(cell_v, 1.0f - cell_v) * spacing;
    const float edge = std::min(edge_u, edge_v);
    const float line_half = std::max(0.0f, border * 0.5f);

    if (shape == "circle") {
        const float dx = cell_u - 0.5f;
        const float dy = cell_v - 0.5f;
        const float radius = std::sqrt(dx * dx + dy * dy) * spacing;
        const float circle_radius = spacing * 0.5f;
        return 1.0f - smoothstep(circle_radius - line_half, circle_radius + line_half, radius);
    } else if (shape == "hexagon") {
        const float dx = cell_u - 0.5f;
        const float dy = cell_v - 0.5f;
        const float ax = std::abs(dx);
        const float ay = std::abs(dy);
        const float dist = std::max(ax * 0.8660254f + ay * 0.5f, ay) * spacing;
        const float hex_radius = spacing * 0.5f;
        return 1.0f - smoothstep(hex_radius - line_half, hex_radius + line_half, dist);
    } else if (shape == "triangle") {
        const float dx = cell_u - 0.5f;
        const float dy = cell_v - 0.5f;
        const float ax = std::abs(dx);
        const float ay = std::abs(dy);
        const float dist = std::max(ax * 1.1547005f + ay * 0.6666667f, ay) * spacing;
        const float tri_radius = spacing * 0.5f;
        return 1.0f - smoothstep(tri_radius - line_half, tri_radius + line_half, dist);
    } else { // default square grid lines
        return 1.0f - smoothstep(line_half, line_half + 1.0f, edge);
    }
}

// --- Galaxy Math Helpers ---
inline float Hash21(float x, float y) {
    float px = std::fmod(x * 123.34f, 1.0f); if (px < 0) px += 1.0f;
    float py = std::fmod(y * 456.21f, 1.0f); if (py < 0) py += 1.0f;
    float d = px * (px + 45.32f) + py * (py + 45.32f);
    px += d; py += d;
    float res = std::fmod(px * py, 1.0f); if (res < 0) res += 1.0f;
    return res;
}

inline float tri(float x) { return std::abs(std::fmod(x, 1.0f) * 2.0f - 1.0f); }
inline float tris(float x) { 
    float t = std::fmod(x, 1.0f); if (t < 0) t += 1.0f;
    return 1.0f - smoothstep(0.0f, 1.0f, std::abs(2.0f * t - 1.0f)); 
}
inline float trisn(float x) {
    float t = std::fmod(x, 1.0f); if (t < 0) t += 1.0f;
    return 2.0f * (1.0f - smoothstep(0.0f, 1.0f, std::abs(2.0f * t - 1.0f))) - 1.0f;
}

inline Color hsv2rgb(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    int i = static_cast<int>(h * 6.0f);
    float f = h * 6.0f - static_cast<float>(i);
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return {r, g, b, 1.0f};
}

inline float Star(float uvx, float uvy, float glowIntensity, float flare) {
    float d = std::sqrt(uvx * uvx + uvy * uvy);
    float m = (0.05f * glowIntensity) / (d + 0.001f);
    
    // Cross rays
    float rays1 = std::max(0.0f, 1.0f - std::abs(uvx * uvy * 1000.0f));
    m += rays1 * flare * glowIntensity;
    
    // Rotate 45 deg
    float ruvx = 0.7071f * uvx - 0.7071f * uvy;
    float ruvy = 0.7071f * uvx + 0.7071f * uvy;
    float rays2 = std::max(0.0f, 1.0f - std::abs(ruvx * ruvy * 1000.0f));
    m += rays2 * 0.3f * flare * glowIntensity;
    
    m *= smoothstep(1.0f, 0.2f, d);
    return m;
}

inline Color StarLayer(float uvx, float uvy, float t, float starSpeed, float density, float hueShift, float saturation, float glowIntensity, float twinkleIntensity) {
    Color col = {0.0f, 0.0f, 0.0f, 0.0f};
    float id_x = std::floor(uvx);
    float id_y = std::floor(uvy);
    float gv_x = uvx - id_x - 0.5f;
    float gv_y = uvy - id_y - 0.5f;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float ox = static_cast<float>(x);
            float oy = static_cast<float>(y);
            float seed = Hash21(id_x + ox, id_y + oy);
            float size = std::fmod(seed * 345.32f, 1.0f);
            float gloss = tri(starSpeed / (3.0f * seed + 1.0f));
            float flare = smoothstep(0.9f, 1.0f, size) * gloss;
            
            float r = smoothstep(0.2f, 1.0f, Hash21(id_x + ox + 1.0f, id_y + oy + 1.0f)) + 0.2f;
            float b = smoothstep(0.2f, 1.0f, Hash21(id_x + ox + 3.0f, id_y + oy + 3.0f)) + 0.2f;
            float g = std::min(r, b) * seed;
            
            float hue = std::atan2(g - r, b - r) / 6.2831853f + 0.5f;
            hue = std::fmod(hue + hueShift / 360.0f, 1.0f); if (hue < 0) hue += 1.0f;
            
            float base_luma = r * 0.299f + g * 0.587f + b * 0.114f;
            float sat = std::sqrt((r - base_luma) * (r - base_luma) + (g - base_luma) * (g - base_luma) + (b - base_luma) * (b - base_luma)) * saturation;
            float val = std::max({r, g, b});
            
            Color base = hsv2rgb(hue, sat, val);
            
            float pad_x = tris(seed * 34.0f + t * 0.1f) - 0.5f;
            float pad_y = tris(seed * 38.0f + t * 0.033f) - 0.5f;
            
            float star = Star(gv_x - ox - pad_x, gv_y - oy - pad_y, glowIntensity, flare);
            float tw = trisn(t + seed * 6.2831f) * 0.5f + 1.0f;
            tw = 1.0f * (1.0f - twinkleIntensity) + tw * twinkleIntensity;
            star *= tw;
            
            col.r += star * size * base.r;
            col.g += star * size * base.g;
            col.b += star * size * base.b;
        }
    }
    return col;
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

    // Ripple & Interaction Params
    const float ripple = static_cast<float>(spec.ripple_intensity.value.value_or(0.0));
    const float mouse_infl = static_cast<float>(spec.mouse_influence.value.value_or(0.0));
    const float mouse_rad = static_cast<float>(spec.mouse_radius.value.value_or(0.5));
    const float mx = spec.mouse_x;
    const float my = spec.mouse_y;
    
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
    const float softness = static_cast<float>(spec.softness.value.value_or(0.0));
    const float glow = static_cast<float>(spec.glow_intensity.value.value_or(0.0));
    const float inv_gamma = 1.0f / gamma;
    
    // Pattern type detection
    const bool is_aura = (spec.kind == "aura" || spec.kind == "nebula");
    const bool is_grid = (spec.kind == "grid");
    const bool is_grid_lines = (spec.kind == "grid_lines");
    const bool is_stars = (spec.kind == "stars");
    const bool is_stripes = (spec.kind == "stripes");
    const bool is_waves = (spec.kind == "waves");
    const bool is_galaxy = (spec.kind == "galaxy");
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

            // STAGE 1.5: SOFTNESS (coordinate jitter for soft/blurry effect)
            if (softness > 0.0f) {
                float jitter_u = (noise.noise2d(u * 10.0f, v * 10.0f) - 0.5f) * softness * 0.1f;
                float jitter_v = (noise.noise2d(v * 10.0f + 0.5f, u * 10.0f + 0.5f) - 0.5f) * softness * 0.1f;
                u += jitter_u;
                v += jitter_v;
            }

            // STAGE 1.6: RIPPLE & MOUSE (Radial distortion)
            if (ripple > 0.0f || mouse_infl > 0.0f) {
                const float aspect = inv_comp_h / inv_comp_w;
                
                // Central Ripple
                if (ripple > 0.0f) {
                    float du = u * 2.0f - 1.0f;
                    float dv = v * 2.0f - 1.0f;
                    du *= aspect;
                    float dist = std::sqrt(du * du + dv * dv);
                    float func = std::sin(3.14159f * (t - dist * 2.0f));
                    u += du * func * ripple * 0.1f;
                    v += dv * func * ripple * 0.1f;
                }

                // Mouse Interaction
                if (mouse_infl > 0.0f) {
                    float mu = mx * 2.0f - 1.0f;
                    float mv = my * 2.0f - 1.0f;
                    float du = (u * 2.0f - 1.0f) * aspect - mu * aspect;
                    float dv = (v * 2.0f - 1.0f) - mv;
                    float dist = std::sqrt(du * du + dv * dv);
                    
                    float infl = mouse_infl * std::exp(-dist * dist / (mouse_rad * mouse_rad));
                    float wave = std::sin(3.14159f * (t * 2.0f - dist * 4.0f)) * infl;
                    
                    if (dist > 0.0001f) {
                        u += (du / dist) * wave * 0.05f;
                        v += (dv / dist) * wave * 0.05f;
                    }
                }
            }
            
            // STAGE 2 & 3: PATTERN & COLOR BLEND
            Color final_color;
            if (is_galaxy) {
                const float star_speed_val = static_cast<float>(spec.star_speed.value.value_or(0.5));
                const float density_val = static_cast<float>(spec.density.value.value_or(1.0));
                const float hue_shift_val = static_cast<float>(spec.hue_shift.value.value_or(140.0));
                const float twinkle_val = static_cast<float>(spec.twinkle_intensity.value.value_or(0.3));
                const float rot_speed_val = static_cast<float>(spec.rotation_speed.value.value_or(0.1));
                const float repulsion_val = static_cast<float>(spec.repulsion_strength.value.value_or(2.0));
                const float auto_rep_val = static_cast<float>(spec.auto_center_repulsion.value.value_or(0.0));
                
                float gu = (u - spec.focal_x) * (static_cast<float>(comp_width) / static_cast<float>(comp_height));
                float gv = (v - spec.focal_y);

                if (auto_rep_val > 0.0f) {
                    float d = std::sqrt(gu * gu + gv * gv);
                    float r_val = auto_rep_val / (d + 0.1f);
                    gu += (gu / (d + 0.001f)) * r_val * 0.05f;
                    gv += (gv / (d + 0.001f)) * r_val * 0.05f;
                } else if (mouse_infl > 0.0f) {
                    float mu = (mx - spec.focal_x) * (static_cast<float>(comp_width) / static_cast<float>(comp_height));
                    float mv = (my - spec.focal_y);
                    float md = std::sqrt((gu - mu) * (gu - mu) + (gv - mv) * (gv - mv));
                    float r_val = repulsion_val / (md + 0.1f);
                    gu += ((gu - mu) / (md + 0.001f)) * r_val * 0.05f;
                    gv += ((gv - mv) / (md + 0.001f)) * r_val * 0.05f;
                }

                float a_rot = t * rot_speed_val + static_cast<float>(spec.angle.value.value_or(0.0) * 3.14159 / 180.0);
                float ca_rot = std::cos(a_rot);
                float sa_rot = std::sin(a_rot);
                float final_u = gu * ca_rot - gv * sa_rot;
                float final_v = gu * sa_rot + gv * ca_rot;

                Color galaxy_col = {0.0f, 0.0f, 0.0f, 0.0f};
                for (float i = 0.0f; i < 1.0f; i += 0.25f) {
                    float d_layer = std::fmod(i + star_speed_val * t * 0.1f, 1.0f);
                    float l_scale = 20.0f * density_val * (1.0f - d_layer) + 0.5f * density_val * d_layer;
                    float f_fade = d_layer * smoothstep(1.0f, 0.9f, d_layer);
                    Color l_col = StarLayer(final_u * l_scale + i * 453.32f, final_v * l_scale + i * 453.32f, t, star_speed_val, density_val, hue_shift_val, saturation, glow, twinkle_val);
                    galaxy_col.r += l_col.r * f_fade;
                    galaxy_col.g += l_col.g * f_fade;
                    galaxy_col.b += l_col.b * f_fade;
                }
                final_color.r = galaxy_col.r;
                final_color.g = galaxy_col.g;
                final_color.b = galaxy_col.b;
                if (spec.transparent) {
                    float luma_gal = galaxy_col.r * 0.299f + galaxy_col.g * 0.587f + galaxy_col.b * 0.114f;
                    final_color.a = std::min(1.0f, luma_gal * 1.5f);
                } else {
                    final_color.a = 1.0f;
                }
            } else {
                float value = 0.0f;
                if (is_aura) {
                    float n1 = noise.noise3d(u * freq, v * freq, t * 0.2f);
                    float n2 = noise.noise3d(u * freq * 2.0f + n1, v * freq * 2.0f, t * 0.5f) * octave_decay;
                    value = (n2 + 1.0f) * 0.5f * amp;
                } else if (is_grid) {
                    float rad = static_cast<float>(spec.angle.value.value_or(0.0) * 3.14159265358979323846 / 180.0);
                    float grid_u = u + t * std::cos(rad) * 0.5f;
                    float grid_v = v + t * std::sin(rad) * 0.5f;
                    value = generate_shape(grid_u, grid_v, shape, spacing, border, static_cast<float>(comp_width), static_cast<float>(comp_height));
                } else if (is_grid_lines) {
                    float gu_lines = std::fmod(u * static_cast<float>(comp_width) / spacing, 1.0f);
                    if (gu_lines < 0) gu_lines += 1.0f;
                    float gv_lines = std::fmod(v * static_cast<float>(comp_height) / spacing, 1.0f);
                    if (gv_lines < 0) gv_lines += 1.0f;
                    float d_u = std::min(gu_lines, 1.0f - gu_lines) * spacing;
                    float d_v = std::min(gv_lines, 1.0f - gv_lines) * spacing;
                    value = (d_u < border || d_v < border) ? 1.0f : 0.0f;
                    if (glow > 0.0f) {
                        float glow_u = std::exp(-d_u * 0.5f);
                        float glow_v = std::exp(-d_v * 0.5f);
                        value = std::max(value, (glow_u + glow_v) * glow);
                    }
                    value *= amp;
                } else if (is_stars) {
                    float n = noise.noise3d(u * freq * 100.0f, v * freq * 100.0f, 0.0f);
                    if (n > 0.8f) {
                        float twinkle_st = std::sin(t * 3.0f + n * 10.0f) * 0.5f + 0.5f;
                        value = ((n - 0.8f) / 0.2f) * twinkle_st * amp;
                    }
                } else if (is_stripes) {
                    float rad = static_cast<float>(spec.angle.value.value_or(0.0) * 3.14159265358979323846 / 180.0);
                    float su = u * std::cos(rad) - v * std::sin(rad);
                    value = std::sin(su * freq * 10.0f + t) * 0.5f + 0.5f;
                    value = std::pow(value, 10.0f) * amp;
                } else if (is_waves) {
                    float w1 = std::sin(u * freq * 10.0f + t) * 0.5f + 0.5f;
                    float w2 = std::sin(v * freq * 8.0f - t * 1.5f) * 0.5f + 0.5f;
                    value = (w1 * w2) * amp;
                } else {
                    value = (noise.noise3d(u * freq * scale, v * freq * scale, t) + 1.0f) * 0.5f * amp;
                }

                if (has_color_c) {
                    if (value < 0.5f) {
                        float t_blend = value * 2.0f;
                        final_color = Color::lerp(col_a, col_b, t_blend);
                    } else {
                        float t_blend = (value - 0.5f) * 2.0f;
                        final_color = Color::lerp(col_b, col_c, t_blend);
                    }
                } else {
                    final_color = Color::lerp(col_a, col_b, value);
                }
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
