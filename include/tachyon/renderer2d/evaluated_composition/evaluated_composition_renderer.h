#pragma once

#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/runtime/core/graph/render_graph.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <vector>

namespace tachyon {
namespace renderer2d {

class CPURasterizer;
struct RenderContext2D;

std::vector<DrawCommand2D> build_draw_commands_from_evaluated_state(
    const scene::EvaluatedCompositionState& state);

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context);

} // namespace renderer2d
} // namespace tachyon