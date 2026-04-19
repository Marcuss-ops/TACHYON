#include "tachyon/renderer2d/rasterizer.h"

namespace tachyon {

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task) {
    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = plan.composition.width;
    frame.height = plan.composition.height;
    frame.layer_count = plan.composition.layer_count;
    frame.estimated_draw_ops = plan.composition.layer_count * 4U + 1U;
    frame.backend_name = "cpu-2d-stub";
    frame.cache_key = task.cache_key.value;
    frame.note = "2D raster path is stubbed; draw ops are estimated from the composition summary";
    return frame;
}

} // namespace tachyon
