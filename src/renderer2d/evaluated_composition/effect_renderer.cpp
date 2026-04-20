#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"

namespace tachyon::renderer2d {

void EffectRenderer::applyEffects(
    EffectHost& effects,
    RenderContext& context,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a) {
    (void)effects; (void)context; (void)accum_r; (void)accum_g; (void)accum_b; (void)accum_a;
}

void EffectRenderer::applyBlurEffect(
    const std::vector<float>& input_r,
    const std::vector<float>& input_g,
    const std::vector<float>& input_b,
    const std::vector<float>& input_a,
    std::vector<float>& output_r,
    std::vector<float>& output_g,
    std::vector<float>& output_b,
    std::vector<float>& output_a,
    float sigma) {
    (void)input_r; (void)input_g; (void)input_b; (void)input_a;
    (void)output_r; (void)output_g; (void)output_b; (void)output_a;
    (void)sigma;
}

} // namespace tachyon::renderer2d
