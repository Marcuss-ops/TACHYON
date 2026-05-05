#pragma once
#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/renderer2d/raster/rasterizer_ops.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/core/graph/runtime_render_graph.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/output/frame_aov.h"
#include "tachyon/renderer2d/resource/render_context.h"

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
    std::shared_ptr<renderer2d::SurfaceRGBA> surface;
    std::vector<output::FrameAOV> aovs;
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
    std::span<const renderer2d::DrawCommand2D> commands,
    renderer2d::RenderContext2D& context);

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task);

} // namespace tachyon
