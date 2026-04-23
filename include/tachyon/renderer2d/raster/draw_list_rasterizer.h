#pragma once

#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/renderer2d/raster/rasterizer.h"


namespace tachyon {

RasterizedFrame2D render_draw_list_2d(
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const renderer2d::DrawList2D& draw_list);

} // namespace tachyon
