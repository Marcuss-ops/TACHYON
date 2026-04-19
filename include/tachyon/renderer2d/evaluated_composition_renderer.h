#pragma once

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/runtime/render_graph.h"
#include "tachyon/timeline/evaluated_state.h"

#include <vector>

namespace tachyon {

std::vector<renderer2d::DrawCommand2D> build_draw_commands_from_evaluated_state(
    const timeline::EvaluatedCompositionState& state);

RasterizedFrame2D render_evaluated_composition_2d(
    const timeline::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task);

} // namespace tachyon
