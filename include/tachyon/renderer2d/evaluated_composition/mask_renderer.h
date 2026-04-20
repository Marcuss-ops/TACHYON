#pragma once

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/render_context.h"
#include "tachyon/core/scene/evaluated_state.h"

#include <vector>

namespace tachyon {
namespace renderer2d {

struct RenderContext;

class MaskRenderer {
public:
    static void applyMask(
        const scene::EvaluatedCompositionState& state,
        RenderContext& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
    
private:
    static void computeMaskBounds(
        const scene::EvaluatedLayerState& layer_state,
        int& x0, int& y0, int& x1, int& y1);
};

} // namespace renderer2d
} // namespace tachyon