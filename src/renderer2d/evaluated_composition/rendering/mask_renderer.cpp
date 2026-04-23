#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

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
    
    // In AE, masks are evaluated per-layer. 
    // This renderer-level hook modulates the final accumulation.
    
    for (const auto& layer : state.layers) {
        if (layer.masks.empty()) continue;
        
        const int width = context.width;
        const int height = context.height;

        for (const auto& mask : layer.masks) {
            // 1. Compute bounds for this mask to optimize
            int x0, y0, x1, y1;
            computeMaskBounds(layer, x0, y0, x1, y1);
            
            // Ensure bounds are within composition
            x0 = std::max(0, x0); y0 = std::max(0, y0);
            x1 = std::min(width, x1); y1 = std::min(height, y1);

            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    const std::size_t idx = static_cast<std::size_t>(y * width + x);
                    if (idx >= accum_a.size()) continue;

                    // 2. Evaluate Distance to Bezier Path (SDF)
                    // Placeholder for actual SDF evaluation logic
                    float dist = 0.0f; // negative inside, positive outside
                    
                    // 3. Apply Per-Vertex Feathering (Alpha Ramp)
                    // In a production implementation, feather values are interpolated along the path
                    float feather_in = 0.0f; 
                    float feather_out = 10.0f; // Default 10px feather
                    
                    if (!mask.vertices.empty()) {
                        feather_in = mask.vertices[0].feather_inner;
                        feather_out = mask.vertices[0].feather_outer;
                    }

                    // Smooth alpha transition: 
                    // Alpha is 1.0 inside (dist < -feather_in), 
                    // 0.0 outside (dist > feather_out),
                    // ramped in between.
                    float mask_alpha = 0.0f;
                    if (dist < -feather_in) {
                        mask_alpha = 1.0f;
                    } else if (dist < feather_out) {
                        float range = feather_out + feather_in;
                        if (range > 1e-6f) {
                            mask_alpha = 1.0f - (dist + feather_in) / range;
                        }
                    }
                    
                    if (mask.is_inverted) mask_alpha = 1.0f - mask_alpha;

                    // 4. Combine with accumulation (Intersects by default)
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
    (void)layer_state;
    
    // Bounds computation for optimization
    x0 = 0; y0 = 0; x1 = 1920; y1 = 1080;
}

} // namespace tachyon::renderer2d
