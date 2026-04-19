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

float edge_function(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

Color sample_texture_nearest(const SurfaceRGBA& texture, float u, float v, Color tint) {
    const std::uint32_t src_width = texture.width();
    const std::uint32_t src_height = texture.height();
    if (src_width == 0U || src_height == 0U) {
        return Color::transparent();
    }

    const float clamped_u = std::clamp(u, 0.0F, 1.0F);
    const float clamped_v = std::clamp(v, 0.0F, 1.0F);
    const std::uint32_t src_x = std::min(src_width - 1U, static_cast<std::uint32_t>(clamped_u * static_cast<float>(src_width - 1U) + 0.5F));
    const std::uint32_t src_y = std::min(src_height - 1U, static_cast<std::uint32_t>(clamped_v * static_cast<float>(src_height - 1U) + 0.5F));
    return multiply_premultiplied(texture.get_pixel(src_x, src_y), tint);
}

void rasterize_textured_triangle(
    Framebuffer& fb,
    const SurfaceRGBA& texture,
    const TexturedVertex2D& a,
    const TexturedVertex2D& b,
    const TexturedVertex2D& c,
    Color tint) {

    const float area = edge_function(a.x, a.y, b.x, b.y, c.x, c.y);
    if (std::fabs(area) < 1e-6F) {
        return;
    }

    const float min_x = std::floor(std::min({a.x, b.x, c.x}));
    const float max_x = std::ceil(std::max({a.x, b.x, c.x}));
    const float min_y = std::floor(std::min({a.y, b.y, c.y}));
    const float max_y = std::ceil(std::max({a.y, b.y, c.y}));

    for (int y = static_cast<int>(min_y); y <= static_cast<int>(max_y); ++y) {
        for (int x = static_cast<int>(min_x); x <= static_cast<int>(max_x); ++x) {
            const float px = static_cast<float>(x) + 0.5F;
            const float py = static_cast<float>(y) + 0.5F;

            const float w0 = edge_function(b.x, b.y, c.x, c.y, px, py);
            const float w1 = edge_function(c.x, c.y, a.x, a.y, px, py);
            const float w2 = edge_function(a.x, a.y, b.x, b.y, px, py);

            const bool inside =
                (area > 0.0F && w0 >= 0.0F && w1 >= 0.0F && w2 >= 0.0F) ||
                (area < 0.0F && w0 <= 0.0F && w1 <= 0.0F && w2 <= 0.0F);
            if (!inside) {
                continue;
            }

            const float inv_area = 1.0F / area;
            const float alpha = w0 * inv_area;
            const float beta = w1 * inv_area;
            const float gamma = w2 * inv_area;

            const float u = alpha * a.u + beta * b.u + gamma * c.u;
            const float v = alpha * a.v + beta * b.v + gamma * c.v;

            if (x >= 0 && y >= 0) {
                fb.blend_pixel(static_cast<std::uint32_t>(x),
                               static_cast<std::uint32_t>(y),
                               sample_texture_nearest(texture, u, v, tint));
            }
        }
    }
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
    if (quad.texture == nullptr) {
        return;
    }

    if (!quad.use_custom_vertices) {
        if (quad.width <= 0 || quad.height <= 0) {
            return;
        }

        const TexturedVertex2D v0{static_cast<float>(quad.x), static_cast<float>(quad.y), 0.0F, 0.0F};
        const TexturedVertex2D v1{static_cast<float>(quad.x + quad.width), static_cast<float>(quad.y), 1.0F, 0.0F};
        const TexturedVertex2D v2{static_cast<float>(quad.x + quad.width), static_cast<float>(quad.y + quad.height), 1.0F, 1.0F};
        const TexturedVertex2D v3{static_cast<float>(quad.x), static_cast<float>(quad.y + quad.height), 0.0F, 1.0F};
        rasterize_textured_triangle(fb, *quad.texture, v0, v1, v2, quad.tint);
        rasterize_textured_triangle(fb, *quad.texture, v0, v2, v3, quad.tint);
        return;
    }

    rasterize_textured_triangle(fb, *quad.texture, quad.vertices[0], quad.vertices[1], quad.vertices[2], quad.tint);
    rasterize_textured_triangle(fb, *quad.texture, quad.vertices[0], quad.vertices[2], quad.vertices[3], quad.tint);
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
    frame.note = "Public 2D pixel renderer producing SurfaceRGBA with premultiplied alpha, opacity, textured quads, and per-command clip";
    frame.surface.emplace(static_cast<std::uint32_t>(std::max<std::int64_t>(0, frame.width)),
                          static_cast<std::uint32_t>(std::max<std::int64_t>(0, frame.height)));

    if (!frame.surface.has_value()) {
        return frame;
    }

    frame.surface->clear(renderer2d::Color::transparent());
    frame.surface->reset_clip_rect();

    for (const renderer2d::DrawCommand2D& command : commands) {
        if (command.has_clip_rect) {
            frame.surface->set_clip_rect(command.clip_rect);
        } else {
            frame.surface->reset_clip_rect();
        }

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

    frame.surface->reset_clip_rect();
    return frame;
}

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task) {
    return render_frame_2d(plan, task, {});
}

} // namespace tachyon
