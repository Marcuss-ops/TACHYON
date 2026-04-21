#pragma once

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/render_context.h"
#include "tachyon/core/scene/evaluated_state.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace tachyon {
namespace renderer2d {

class CPURasterizer;
struct RenderContext2D;

class LayerRenderer {
public:
    static void renderLayer(
        const scene::EvaluatedLayerState& layer_state,
        const scene::EvaluatedCompositionState& comp_state,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
    
private:
    static void renderSolidLayer(
        const scene::EvaluatedLayerState& layer_state,
        const scene::EvaluatedCompositionState& comp_state,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
    
    static void renderShapeLayer(
        const scene::EvaluatedLayerState& layer_state,
        const scene::EvaluatedCompositionState& comp_state,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
    
    static void renderImageLayer(
        const scene::EvaluatedLayerState& layer_state,
        const scene::EvaluatedCompositionState& comp_state,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
    
    static void renderTextLayer(
        const scene::EvaluatedLayerState& layer_state,
        const scene::EvaluatedCompositionState& comp_state,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);
};

} // namespace renderer2d
} // namespace tachyon