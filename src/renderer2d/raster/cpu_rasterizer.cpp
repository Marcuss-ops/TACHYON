#include "tachyon/renderer2d/raster/rasterizer.h"

#ifdef TACHYON_TRACY_ENABLED
#include <tracy/Tracy.hpp>
#endif

#include <algorithm>
#include <cmath>
#include <vector>
#include <chrono>
#include <iostream>

namespace tachyon {
namespace renderer2d {
namespace {

Color multiply_premultiplied(Color sample, Color tint) {
    return Color{
        sample.r * tint.r,
        sample.g * tint.g,
        sample.b * tint.b,
        sample.a * tint.a
    };
}

float edge_function(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

Color sample_texture_bilinear(const SurfaceRGBA& texture, float u, float v, Color tint) {
    const float src_width = static_cast<float>(texture.width());
    const float src_height = static_cast<float>(texture.height());
    if (src_width <= 0.0F || src_height <= 0.0F) {
        return Color::transparent();
    }

    const float u_clamped = std::clamp(u, 0.0f, 1.0f);
    const float v_clamped = std::clamp(v, 0.0f, 1.0f);
    const std::uint32_t x = static_cast<std::uint32_t>(std::lround(u_clamped * (src_width - 1.0f)));
    const std::uint32_t y = static_cast<std::uint32_t>(std::lround(v_clamped * (src_height - 1.0f)));
    return multiply_premultiplied(texture.get_pixel(x, y), tint);
}

void rasterize_textured_triangle(
    Framebuffer& fb,
    const SurfaceRGBA& texture,
    const TexturedVertex2D& a,
    const TexturedVertex2D& b,
    const TexturedVertex2D& c,
    Color tint) {

    const float area = edge_function(a.x, a.y, b.x, b.y, c.x, c.y);
    if (std::abs(area) < 1e-6F) {
        return;
    }

    const float min_x = std::floor(std::min({a.x, b.x, c.x}));
    const float max_x = std::ceil(std::max({a.x, b.x, c.x}));
    const float min_y = std::floor(std::min({a.y, b.y, c.y}));
    const float max_y = std::ceil(std::max({a.y, b.y, c.y}));

    const float aw = a.inv_w;
    const float bw = b.inv_w;
    const float cw = c.inv_w;
    const float auw = a.u * aw;
    const float avw = a.v * aw;
    const float buw = b.u * bw;
    const float bvw = b.v * bw;
    const float cuw = c.u * cw;
    const float cvw = c.v * cw;

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

            const float interp_inv_w = alpha * aw + beta * bw + gamma * cw;
            const float interp_uw = alpha * auw + beta * buw + gamma * cuw;
            const float interp_vw = alpha * avw + beta * bvw + gamma * cvw;

            const float u = interp_uw / interp_inv_w;
            const float v = interp_vw / interp_inv_w;

            fb.blend_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 
                           sample_texture_bilinear(texture, u, v, tint));
        }
    }
}

} // namespace anonymous

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

        const TexturedVertex2D v0{static_cast<float>(quad.x), static_cast<float>(quad.y), 0.0F, 0.0F, 1.0f};
        const TexturedVertex2D v1{static_cast<float>(quad.x + quad.width), static_cast<float>(quad.y), 1.0F, 0.0F, 1.0f};
        const TexturedVertex2D v2{static_cast<float>(quad.x + quad.width), static_cast<float>(quad.y + quad.height), 1.0F, 1.0F, 1.0f};
        const TexturedVertex2D v3{static_cast<float>(quad.x), static_cast<float>(quad.y + quad.height), 0.0F, 1.0F, 1.0f};
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

#ifdef TACHYON_TRACY_ENABLED
    ZoneScopedN("render_frame_2d");
#endif

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = plan.composition.width;
    frame.height = plan.composition.height;
    frame.layer_count = plan.composition.layer_count;
    frame.estimated_draw_ops = commands.size();
    frame.backend_name = "cpu-2d-surface-rgba";
    frame.cache_key = task.cache_key.value;
    frame.note = "High-quality perspective-correct 2D rasterizer";
    frame.surface = std::make_shared<renderer2d::SurfaceRGBA>(
        static_cast<std::uint32_t>(std::max<std::int64_t>(1, frame.width)),
        static_cast<std::uint32_t>(std::max<std::int64_t>(1, frame.height)));

    if (!frame.surface) {
        return frame;
    }

    frame.surface->clear(renderer2d::Color::transparent());
    
    renderer2d::RectI active_clip{0, 0, static_cast<int>(frame.width), static_cast<int>(frame.height)};
    frame.surface->set_clip_rect(active_clip);

    for (const auto& cmd : commands) {
        if (cmd.clip.has_value()) {
            frame.surface->set_clip_rect(*cmd.clip);
        } else {
            frame.surface->set_clip_rect(active_clip);
        }

        switch (cmd.kind) {
            case renderer2d::DrawCommandKind::Clear:
                if (cmd.clear) frame.surface->clear(cmd.clear->color);
                break;
            case renderer2d::DrawCommandKind::SolidRect:
                if (cmd.solid_rect) {
                    renderer2d::RectPrimitive p;
                    p.x = cmd.solid_rect->rect.x;
                    p.y = cmd.solid_rect->rect.y;
                    p.width = cmd.solid_rect->rect.width;
                    p.height = cmd.solid_rect->rect.height;
                    p.color = cmd.solid_rect->color;
                    renderer2d::CPURasterizer::draw_rect(*frame.surface, p);
                }
                break;
            case renderer2d::DrawCommandKind::Line:
                if (cmd.line) {
                    renderer2d::LinePrimitive p;
                    p.x0 = cmd.line->x0;
                    p.y0 = cmd.line->y0;
                    p.x1 = cmd.line->x1;
                    p.y1 = cmd.line->y1;
                    p.color = cmd.line->color;
                    renderer2d::CPURasterizer::draw_line(*frame.surface, p);
                }
                break;
            case renderer2d::DrawCommandKind::TexturedQuad:
                if (cmd.textured_quad) {
                    const auto& t = cmd.textured_quad;
                    if (t->texture.surface) {
                        renderer2d::TexturedQuadPrimitive p;
                        p.texture = t->texture.surface;
                        p.use_custom_vertices = true;
                        p.vertices[0] = {t->p0.x, t->p0.y, 0, 0, t->w0};
                        p.vertices[1] = {t->p1.x, t->p1.y, 1, 0, t->w1};
                        p.vertices[2] = {t->p2.x, t->p2.y, 1, 1, t->w2};
                        p.vertices[3] = {t->p3.x, t->p3.y, 0, 1, t->w3};
                        p.tint = renderer2d::Color::white();
                        renderer2d::CPURasterizer::draw_textured_quad(*frame.surface, p);
                    }
                }
                break;
            default:
                break;
        }
    }

    frame.surface->reset_clip_rect();
    return frame;
}

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task) {
    auto frame = render_frame_2d(plan, task, {});
    frame.estimated_draw_ops = plan.composition.layer_count * 5;
    return frame;
}

} // namespace tachyon
