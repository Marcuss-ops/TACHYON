#pragma once

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/render_context.h"
#include "tachyon/runtime/core/render_graph.h"
#include "tachyon/core/scene/evaluated_state.h"

#include <vector>

namespace tachyon {

std::vector<renderer2d::DrawCommand2D> build_draw_commands_from_evaluated_state(
    const scene::EvaluatedCompositionState& state);

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext2D& context);

} // namespace tachyon
