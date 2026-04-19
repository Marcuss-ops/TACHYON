#include "tachyon/renderer2d/rasterizer.h"
#include <cmath>

namespace tachyon {
namespace renderer2d {

void CPURasterizer::draw_rect(Framebuffer& fb, const RectPrimitive& rect) {
    // 1. Clipping
    int x_start = std::max(0, rect.x);
    int y_start = std::max(0, rect.y);
    int x_end = std::min(static_cast<int>(fb.width()), rect.x + rect.width);
    int y_end = std::min(static_cast<int>(fb.height()), rect.y + rect.height);

    if (x_start >= x_end || y_start >= y_end) return;

    // 2. Rasterization
    for (int y = y_start; y < y_end; ++y) {
        for (int x = x_start; x < x_end; ++x) {
            if (rect.color.a == 255) {
                fb.set_pixel(x, y, rect.color);
            } else {
                Color dest = fb.get_pixel(x, y);
                fb.set_pixel(x, y, Blender::composite_premultiplied(rect.color, dest));
            }
        }
    }
}

void CPURasterizer::draw_line(Framebuffer& fb, const LinePrimitive& line) {
    // Bresenham's line algorithm
    int x0 = line.x0;
    int y0 = line.y0;
    int x1 = line.x1;
    int y1 = line.y1;

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    uint32_t fb_w = fb.width();
    uint32_t fb_h = fb.height();

    while (true) {
        // Pixel-level clipping
        if (x0 >= 0 && x0 < static_cast<int>(fb_w) && y0 >= 0 && y0 < static_cast<int>(fb_h)) {
            if (line.color.a == 255) {
                fb.set_pixel(x0, y0, line.color);
            } else {
                Color dest = fb.get_pixel(x0, y0);
                fb.set_pixel(x0, y0, Blender::composite_premultiplied(line.color, dest));
            }
        }

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
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
    frame.backend_name = "cpu-2d-refined";
    frame.cache_key = task.cache_key.value;
    frame.note = "2D raster path now supports Rect and Line primitives via CPURasterizer";
    return frame;
}

} // namespace tachyon
