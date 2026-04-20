#pragma once
#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/core/render_graph.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace tachyon {

struct RasterizedFrame2D {
    std::int64_t frame_number{0};
    std::int64_t width{0};
    std::int64_t height{0};
    std::size_t layer_count{0};
    std::size_t estimated_draw_ops{0};
    std::string backend_name;
    std::string cache_key;
    std::string note;
    std::optional<renderer2d::SurfaceRGBA> surface;
};

namespace renderer2d {

class CPURasterizer {
public:
    static void draw_rect(Framebuffer& fb, const RectPrimitive& rect);
    static void draw_line(Framebuffer& fb, const LinePrimitive& line);
    static void draw_textured_quad(Framebuffer& fb, const TexturedQuadPrimitive& quad);
};

} // namespace renderer2d

RasterizedFrame2D render_frame_2d(
    const RenderPlan& plan,
    const FrameRenderTask& task,
    std::span<const renderer2d::DrawCommand2D> commands);

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task);

} // namespace tachyon
