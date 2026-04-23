#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include "tachyon/renderer2d/raster/path/path_flattener.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

namespace {

inline float point_segment_dist(const math::Vector2& p, const math::Vector2& a, const math::Vector2& b, float& out_t) {
    math::Vector2 ab = b - a;
    math::Vector2 ap = p - a;
    float len2 = ab.length_squared();
    float t = len2 > 1e-6f ? std::clamp(math::Vector2::dot(ap, ab) / len2, 0.0f, 1.0f) : 0.0f;
    out_t = t;
    math::Vector2 proj = a + ab * t;
    return (p - proj).length();
}

inline float signed_dist_to_mask(const math::Vector2& p, const MaskPath& mask, float& out_feather_inner, float& out_feather_outer) {
    if (mask.vertices.size() < 2) return 1e6f;

    float best_d = 1e6f;
    int winding = 0;
    out_feather_inner = 0.0f;
    out_feather_outer = 0.0f;

    const std::size_t n = mask.vertices.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        const auto& va = mask.vertices[i];
        const auto& vb = mask.vertices[i + 1];
        const math::Vector2 a = va.position;
        const math::Vector2 b = vb.position;

        float t = 0.0f;
        float d = point_segment_dist(p, a, b, t);
        if (d < best_d) {
            best_d = d;
            out_feather_inner = va.feather_inner * (1.0f - t) + vb.feather_inner * t;
            out_feather_outer = va.feather_outer * (1.0f - t) + vb.feather_outer * t;
        }

        if (((a.y > p.y) != (b.y > p.y)) && (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + 1e-12f) + a.x)) {
            winding = (winding + 1) % 2;
        }
    }

    if (mask.is_closed && n >= 2) {
        const auto& va = mask.vertices.back();
        const auto& vb = mask.vertices.front();
        const math::Vector2 a = va.position;
        const math::Vector2 b = vb.position;
        float t = 0.0f;
        float d = point_segment_dist(p, a, b, t);
        if (d < best_d) {
            best_d = d;
            out_feather_inner = va.feather_inner * (1.0f - t) + vb.feather_inner * t;
            out_feather_outer = va.feather_outer * (1.0f - t) + vb.feather_outer * t;
        }
        if (((a.y > p.y) != (b.y > p.y)) && (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + 1e-12f) + a.x)) {
            winding = (winding + 1) % 2;
        }
    }

    return winding ? -best_d : best_d;
}

inline float compute_mask_alpha(float signed_dist, float feather_inner, float feather_outer) {
    if (signed_dist < -feather_inner) {
        return 1.0f;
    } else if (signed_dist < feather_outer) {
        float range = feather_outer + feather_inner;
        if (range > 1e-6f) {
            return std::clamp(1.0f - (signed_dist + feather_inner) / range, 0.0f, 1.0f);
        }
        return 1.0f;
    }
    return 0.0f;
}

} // namespace

void MaskRenderer::applyMask(
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a) {
    (void)accum_r;
    (void)accum_g;
    (void)accum_b;
    (void)context;
    
    for (const auto& layer : state.layers) {
        if (layer.masks.empty()) continue;
        
        const int width = context.width;
        const int height = context.height;

        for (const auto& mask : layer.masks) {
            if (mask.vertices.size() < 2) continue;

            int x0, y0, x1, y1;
            computeMaskBounds(layer, x0, y0, x1, y1);
            
            x0 = std::max(0, x0); y0 = std::max(0, y0);
            x1 = std::min(width, x1); y1 = std::min(height, y1);

            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    const std::size_t idx = static_cast<std::size_t>(y * width + x);
                    if (idx >= accum_a.size()) continue;

                    math::Vector2 pixel_pos{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
                    float feather_in = 0.0f, feather_out = 0.0f;
                    float dist = signed_dist_to_mask(pixel_pos, mask, feather_in, feather_out);
                    float mask_alpha = compute_mask_alpha(dist, feather_in, feather_out);
                    
                    if (mask.is_inverted) mask_alpha = 1.0f - mask_alpha;
                    accum_a[idx] *= mask_alpha;
                }
            }
        }
    }
}

void MaskRenderer::applyMaskMotionBlur(
    const std::vector<const scene::EvaluatedCompositionState*>& states,
    RenderContext2D& context,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a,
    int sample_count) {
    (void)context;
    (void)accum_r;
    (void)accum_g;
    (void)accum_b;
    (void)accum_a;
    (void)states;
    (void)sample_count;
    
    if (states.empty() || sample_count <= 0) return;
}

void MaskRenderer::computeMaskBounds(
    const scene::EvaluatedLayerState& layer_state,
    int& x0, int& y0, int& x1, int& y1) {
    // Compute actual bounds from mask vertices, with feather padding
    float min_x = 1e6f, min_y = 1e6f, max_x = -1e6f, max_y = -1e6f;
    bool has_any = false;
    for (const auto& mask : layer_state.masks) {
        for (const auto& v : mask.vertices) {
            float pad = std::max(v.feather_outer, std::abs(v.feather_inner)) + 2.0f;
            min_x = std::min(min_x, v.position.x - pad);
            min_y = std::min(min_y, v.position.y - pad);
            max_x = std::max(max_x, v.position.x + pad);
            max_y = std::max(max_y, v.position.y + pad);
            has_any = true;
        }
    }
    if (!has_any) {
        x0 = 0; y0 = 0; x1 = 0; y1 = 0;
        return;
    }
    x0 = static_cast<int>(std::floor(min_x));
    y0 = static_cast<int>(std::floor(min_y));
    x1 = static_cast<int>(std::ceil(max_x));
    y1 = static_cast<int>(std::ceil(max_y));
}

} // namespace tachyon::renderer2d
