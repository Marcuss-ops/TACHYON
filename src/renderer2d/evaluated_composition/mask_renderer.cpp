#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"

namespace tachyon::renderer2d {

void MaskRenderer::applyMask(
    const scene::EvaluatedCompositionState& state,
    RenderContext& context,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a) {
    (void)state; (void)context; (void)accum_r; (void)accum_g; (void)accum_b; (void)accum_a;
    // Mask application logic would go here
}

void MaskRenderer::computeMaskBounds(
    const scene::EvaluatedLayerState& layer_state,
    int& x0, int& y0, int& x1, int& y1) {
    (void)layer_state;
    x0 = y0 = x1 = y1 = 0;
}

} // namespace tachyon::renderer2d
