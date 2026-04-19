#pragma once
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/render_graph.h"

#include <string>
#include <cstdint>

namespace tachyon {

/**
 * Metadata for a rasterized 2D frame.
 */
struct RasterizedFrame2D {
    std::int64_t frame_number{0};
    std::int64_t width{0};
    std::int64_t height{0};
    std::size_t layer_count{0};
    std::size_t estimated_draw_ops{0};
    std::string backend_name;
    std::string cache_key;
    std::string note;
};

namespace renderer2d {

/**
 * High-level 2D rasterizer using the CPU.
 */
class CPURasterizer {
public:
    static void draw_rect(Framebuffer& fb, const RectPrimitive& rect);
    static void draw_line(Framebuffer& fb, const LinePrimitive& line);
};

} // namespace renderer2d

/**
 * Current stub for the 2D rasterization path.
 */
RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task);

} // namespace tachyon
