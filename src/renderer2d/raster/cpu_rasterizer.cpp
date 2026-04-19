#include "tachyon/renderer2d/rasterizer.h"
#include <cmath>

#include "../surface_rgba.cpp"

namespace tachyon {
namespace renderer2d {

void CPURasterizer::draw_rect(Framebuffer& fb, const RectPrimitive& rect) {
    fb.fill_rect(RectI{rect.x, rect.y, rect.width, rect.height}, rect.color, true);
}

void CPURasterizer::draw_line(Framebuffer& fb, const LinePrimitive& line) {
    int x0 = line.x0;
    int y0 = line.y0;
    int x1 = line.x1;
    int y1 = line.y1;

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        fb.blend_pixel(static_cast<uint32_t>(x0), static_cast<uint32_t>(y0), line.color);

        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

} // namespace renderer2d

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task) {
    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = plan.composition.width;
    frame.height = plan.composition.height;
    frame.layer_count = plan.composition.layer_count;
    frame.estimated_draw_ops = plan.composition.layer_count * 4U + 1U;
    frame.backend_name = "cpu-2d-surface-rgba";
    frame.cache_key = task.cache_key.value;
    frame.note = "2D raster path now uses SurfaceRGBA clear, clip, pixel access, and premultiplied alpha rect fill";
    return frame;
}

} // namespace tachyon
