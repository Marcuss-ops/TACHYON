#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include "tachyon/renderer2d/raster/path/path_flattener.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief FeatheredMaskRenderer: Renders masks with per-vertex variable feather.
 * 
 * This renderer generates inner and outer alpha ramps for each mask edge:
 * - Inner ramp: 1.0 → 0.0 over feather_inner distance (inside the mask)
 * - Outer ramp: 0.0 → 1.0 over feather_outer distance (outside the mask)
 * - Result: inner_ramp * outer_ramp gives smooth feathered edges
 */
class FeatheredMaskRenderer {
public:
    /**
     * @brief Render a single mask with variable feather to an alpha buffer.
     * @param mask The mask path with per-vertex feather data
     * @param width Buffer width
     * @param height Buffer height
     * @param x0, y0, x1, y1 Rendering bounds (clipped to buffer)
     * @param alpha Output alpha buffer (modified in-place, multiplied)
     * @param inverted Whether to invert the mask alpha
     */
    static void renderMask(const MaskPath& mask,
                          int width, int height,
                          int x0, int y0, int x1, int y1,
                          std::vector<float>& alpha,
                          bool inverted = false);
    
    /**
     * @brief Compute signed distance from point to mask edge with feather interpolation.
     * @param p Query point
     * @param mask Mask path
     * @param out_feather_inner Output: interpolated inner feather at closest point
     * @param out_feather_outer Output: interpolated outer feather at closest point
     * @return Signed distance (negative inside, positive outside)
     */
    static float signedDistanceWithFeather(const math::Vector2& p,
                                          const MaskPath& mask,
                                          float& out_feather_inner,
                                          float& out_feather_outer);
    
    /**
     * @brief Compute alpha from signed distance and feather values.
     * @param signed_dist Signed distance to mask edge
     * @param feather_inner Inner feather radius
     * @param feather_outer Outer feather radius
     * @return Alpha value in [0, 1]
     */
    static float computeAlpha(float signed_dist, float feather_inner, float feather_outer);
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

namespace {

inline float point_segment_distance(const math::Vector2& p, 
                                    const math::Vector2& a, 
                                    const math::Vector2& b, 
                                    float& out_t) {
    math::Vector2 ab = b - a;
    math::Vector2 ap = p - a;
    float len2 = ab.length_squared();
    float t = len2 > 1e-6f ? std::clamp(math::Vector2::dot(ap, ab) / len2, 0.0f, 1.0f) : 0.0f;
    out_t = t;
    math::Vector2 proj = a + ab * t;
    return (p - proj).length();
}

} // anonymous namespace

float FeatheredMaskRenderer::signedDistanceWithFeather(
    const math::Vector2& p,
    const MaskPath& mask,
    float& out_feather_inner,
    float& out_feather_outer) {
    
    if (mask.vertices.size() < 2) {
        out_feather_inner = 0.0f;
        out_feather_outer = 0.0f;
        return 1e6f;
    }
    
    float best_dist = 1e6f;
    int winding = 0;
    out_feather_inner = 0.0f;
    out_feather_outer = 0.0f;
    
    const std::size_t n = mask.vertices.size();
    
    // Test each segment
    for (std::size_t i = 0; i + 1 < n; ++i) {
        const auto& va = mask.vertices[i];
        const auto& vb = mask.vertices[i + 1];
        const math::Vector2 a = va.position;
        const math::Vector2 b = vb.position;
        
        float t = 0.0f;
        float dist = point_segment_distance(p, a, b, t);
        
        // Update best distance and interpolate feather values
        if (dist < best_dist) {
            best_dist = dist;
            out_feather_inner = va.feather_inner * (1.0f - t) + vb.feather_inner * t;
            out_feather_outer = va.feather_outer * (1.0f - t) + vb.feather_outer * t;
        }
        
        // Winding number test for inside/outside
        if (((a.y > p.y) != (b.y > p.y)) && 
            (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + 1e-12f) + a.x)) {
            winding = (winding + 1) % 2;
        }
    }
    
    // Close the loop if mask is closed
    if (mask.is_closed && n >= 2) {
        const auto& va = mask.vertices.back();
        const auto& vb = mask.vertices.front();
        const math::Vector2 a = va.position;
        const math::Vector2 b = vb.position;
        
        float t = 0.0f;
        float dist = point_segment_distance(p, a, b, t);
        
        if (dist < best_dist) {
            best_dist = dist;
            out_feather_inner = va.feather_inner * (1.0f - t) + vb.feather_inner * t;
            out_feather_outer = va.feather_outer * (1.0f - t) + vb.feather_outer * t;
        }
        
        if (((a.y > p.y) != (b.y > p.y)) && 
            (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + 1e-12f) + a.x)) {
            winding = (winding + 1) % 2;
        }
    }
    
    // Return signed distance (negative inside, positive outside)
    return winding > 0 ? -best_dist : best_dist;
}

float FeatheredMaskRenderer::computeAlpha(float signed_dist, 
                                          float feather_inner, 
                                          float feather_outer) {
    // Ensure non-negative feather values
    feather_inner = std::max(0.0f, feather_inner);
    feather_outer = std::max(0.0f, feather_outer);
    
    if (signed_dist < -feather_inner) {
        // Fully inside the mask (beyond inner feather)
        return 1.0f;
    } else if (signed_dist < feather_outer) {
        // In the feather region (transition from inside to outside)
        float total_range = feather_inner + feather_outer;
        if (total_range > 1e-6f) {
            // Linear ramp from 1.0 (at -feather_inner) to 0.0 (at feather_outer)
            return std::clamp(1.0f - (signed_dist + feather_inner) / total_range, 0.0f, 1.0f);
        }
        return 1.0f;
    }
    // Fully outside the mask
    return 0.0f;
}

void FeatheredMaskRenderer::renderMask(const MaskPath& mask,
                                       int width, int height,
                                       int x0, int y0, int x1, int y1,
                                       std::vector<float>& alpha,
                                       bool inverted) {
    if (mask.vertices.size() < 2) return;
    
    // Clamp bounds to buffer size
    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(width, x1);
    y1 = std::min(height, y1);
    
    if (x0 >= x1 || y0 >= y1) return;
    
    // Render each pixel in the bounds
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const std::size_t idx = static_cast<std::size_t>(y * width + x);
            if (idx >= alpha.size()) continue;
            
            // Pixel center position
            math::Vector2 pixel_pos{static_cast<float>(x) + 0.5f, 
                                   static_cast<float>(y) + 0.5f};
            
            // Compute signed distance and interpolated feather values
            float feather_in = 0.0f, feather_out = 0.0f;
            float dist = signedDistanceWithFeather(pixel_pos, mask, feather_in, feather_out);
            
            // Compute alpha from distance and feather
            float mask_alpha = computeAlpha(dist, feather_in, feather_out);
            
            // Apply inversion if requested
            if (inverted) {
                mask_alpha = 1.0f - mask_alpha;
            }
            
            // Multiply into existing alpha (stacking masks)
            alpha[idx] *= mask_alpha;
        }
    }
}

// Static wrapper functions for backward compatibility
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
    
    const int width = context.width;
    const int height = context.height;
    
    for (const auto& layer : state.layers) {
        if (layer.masks.empty()) continue;
        
        for (const auto& mask : layer.masks) {
            if (mask.vertices.size() < 2) continue;
            
            // Compute rendering bounds
            int x0, y0, x1, y1;
            computeMaskBounds(layer, x0, y0, x1, y1);
            
            // Render the mask with variable feather
            FeatheredMaskRenderer::renderMask(mask, width, height, 
                                              x0, y0, x1, y1, 
                                              accum_a, mask.is_inverted);
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
    
    if (states.empty() || sample_count <= 0) return;
    
    const int width = context.width;
    const int height = context.height;
    const float weight = 1.0f / static_cast<float>(sample_count);
    
    // Temporary buffer for each sample
    std::vector<float> sample_alpha(width * height, 1.0f);
    
    // Accumulate alpha from each sub-frame sample
    for (const auto* state : states) {
        if (!state) continue;
        
        // Reset sample buffer
        std::fill(sample_alpha.begin(), sample_alpha.end(), 1.0f);
        
        // Apply all masks for this sample
        for (const auto& layer : state->layers) {
            if (layer.masks.empty()) continue;
            
            for (const auto& mask : layer.masks) {
                if (mask.vertices.size() < 2) continue;
                
                int x0, y0, x1, y1;
                computeMaskBounds(layer, x0, y0, x1, y1);
                
                FeatheredMaskRenderer::renderMask(mask, width, height,
                                                  x0, y0, x1, y1,
                                                  sample_alpha, mask.is_inverted);
            }
        }
        
        // Accumulate weighted sample into final buffer
        for (std::size_t i = 0; i < accum_a.size(); ++i) {
            accum_a[i] += sample_alpha[i] * weight;
        }
    }
}

void MaskRenderer::computeMaskBounds(
    const scene::EvaluatedLayerState& layer_state,
    int& x0, int& y0, int& x1, int& y1) {
    // Compute actual bounds from mask vertices, with feather padding
    float min_x = 1e6f, min_y = 1e6f, max_x = -1e6f, max_y = -1e6f;
    bool has_any = false;
    
    for (const auto& mask : layer_state.masks) {
        for (const auto& v : mask.vertices) {
            // Pad by maximum feather extent plus small margin
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
