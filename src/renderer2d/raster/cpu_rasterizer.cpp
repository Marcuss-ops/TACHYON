#include "tachyon/renderer2d/rasterizer.h"

#include <algorithm>
#include <cmath>

namespace tachyon {
namespace renderer2d {
namespace {

Color multiply_premultiplied(Color sample, Color tint) {
    return Color{
        static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.r) * tint.r) / 255U),
        static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.g) * tint.g) / 255U),
        static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.b) * tint.b) / 255U),
        static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.a) * tint.a) / 255U)
    };
}

} // namespace

void CPURasterizer::draw_rect(Framebuffer& fb, const RectPrimitive& rect) {
    fb.fill_rect(RectI{rect.x, rect.y, rect.width, rect.height}, rect.color, true);
}

void CPURasterizer::draw_line(Framebuffer& fb, const LinePrimitive& line) {
    int x0 = line.x0;
    int y0 = line.y0;
    const int x1 = line.x1;
    const int y1 = line.y1;

    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x0 >= 0 && y0 >= 0) {
            fb.blend_pixel(static_cast<uint32_t>(x0), static_cast<uint32_t>(y0), line.color);
        }

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

void CPURasterizer::draw_textured_quad(Framebuffer& fb, const TexturedQuadPrimitive& quad) {
    if (quad.texture == nullptr || quad.width <= 0 || quad.height <= 0) {
        return;
    }

    const std::uint32_t src_width = quad.texture->width();
    const std::uint32_t src_height = quad.texture->height();
    if (src_width == 0U || src_height == 0U) {
        return;
    }

    for (int dy = 0; dy < quad.height; ++dy) {
        const float v = quad.height == 1 ? 0.0F : static_cast<float>(dy) / static_cast<float>(quad.height - 1);
        const std::uint32_t src_y = std::min(src_height - 1U, static_cast<std::uint32_t>(v * static_cast<float>(src_height - 1U)));

        for (int dx = 0; dx < quad.width; ++dx) {
            const float u = quad.width == 1 ? 0.0F : static_cast<float>(dx) / static_cast<float>(quad.width - 1);
            const std::uint32_t src_x = std::min(src_width - 1U, static_cast<std::uint32_t>(u * static_cast<float>(src_width - 1U)));

            const int out_x = quad.x + dx;
            const int out_y = quad.y + dy;
            if (out_x < 0 || out_y < 0) {
                continue;
            }

            const Color sampled = quad.texture->get_pixel(src_x, src_y);
            const Color tinted = multiply_premultiplied(sampled, quad.tint);
            fb.blend_pixel(static_cast<std::uint32_t>(out_x), static_cast<std::uint32_t>(out_y), tinted);
        }
    }
}

} // namespace renderer2d

RasterizedFrame2D render_frame_2d(
    const RenderPlan& plan,
    const FrameRenderTask& task,
    std::span<const renderer2d::DrawCommand2D> commands) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = plan.composition.width;
    frame.height = plan.composition.height;
    frame.layer_count = plan.composition.layer_count;
    frame.estimated_draw_ops = commands.size();
    frame.backend_name = "cpu-2d-surface-rgba";
    frame.cache_key = task.cache_key.value;
    frame.note = "2D frame rasterized into SurfaceRGBA using premultiplied alpha draw commands";
    frame.surface.emplace(static_cast<std::uint32_t>(std::max<std::int64_t>(0, frame.width)),
                          static_cast<std::uint32_t>(std::max<std::int64_t>(0, frame.height)));

    if (!frame.surface.has_value()) {
        return frame;
    }

    frame.surface->clear(renderer2d::Color::transparent());

    for (const renderer2d::DrawCommand2D& command : commands) {
        switch (command.kind) {
            case renderer2d::DrawCommand2D::Kind::Clear:
                frame.surface->clear(command.clear.color);
                break;
            case renderer2d::DrawCommand2D::Kind::Rect:
                renderer2d::CPURasterizer::draw_rect(*frame.surface, command.rect);
                break;
            case renderer2d::DrawCommand2D::Kind::Line:
                renderer2d::CPURasterizer::draw_line(*frame.surface, command.line);
                break;
            case renderer2d::DrawCommand2D::Kind::TexturedQuad:
                renderer2d::CPURasterizer::draw_textured_quad(*frame.surface, command.textured_quad);
                break;
        }
    }

    return frame;
}

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task) {
    return render_frame_2d(plan, task, {});
}

} // namespace tachyon
