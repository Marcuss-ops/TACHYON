#pragma once

#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/runtime/core/render_graph.h"

namespace tachyon {

RasterizedFrame2D render_draw_list_2d(
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const renderer2d::DrawList2D& draw_list);

} // namespace tachyon
