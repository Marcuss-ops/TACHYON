#pragma once

#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/render_context.h"

#include <memory>
#include <vector>

namespace tachyon {
namespace renderer2d {

class EffectRenderer {
public:
    static void applyEffects(
        EffectHost& effects,
        RenderContext& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
    
private:
    static void applyBlurEffect(
        const std::vector<float>& input_r,
        const std::vector<float>& input_g,
        const std::vector<float>& input_b,
        const std::vector<float>& input_a,
        std::vector<float>& output_r,
        std::vector<float>& output_g,
        std::vector<float>& output_b,
        std::vector<float>& output_a,
        float sigma);
};

} // namespace renderer2d
} // namespace tachyon