#include "tachyon/renderer2d/draw_list_rasterizer.h"

#include "tachyon/renderer2d/rasterizer_ops.h"

#include <algorithm>
#include <vector>

namespace tachyon {
using namespace renderer2d;

namespace {

using ::tachyon::renderer2d::BlendMode;
using ::tachyon::renderer2d::Color;
using ::tachyon::renderer2d::LinePrimitive;
using ::tachyon::renderer2d::RectI;
using ::tachyon::renderer2d::SurfaceRGBA;

renderer2d::Color placeholder_textured_color(float opacity) {
    renderer2d::Color color{255, 255, 255, 255};
    color.a = static_cast<std::uint8_t>(opacity <= 0.0f ? 0.0f : (opacity >= 1.0f ? 255.0f : opacity * 255.0f));
    color.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.r) * color.a + 127U) / 255U);
    color.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.g) * color.a + 127U) / 255U);
    color.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.b) * color.a + 127U) / 255U);
    return color;
}

RectI intersect_rects(const RectI& a, const RectI& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0) {
        return RectI{0, 0, 0, 0};
    }
    return RectI{x0, y0, x1 - x0, y1 - y0};
}

Color blend_for_mode(Color src, Color dest, BlendMode mode) {
    return blend_mode_color(src, dest, mode);
}

bool write_pixel(SurfaceRGBA& fb, int x, int y, Color color, BlendMode mode) {
    if (x < 0 || y < 0) {
        return false;
    }

    const uint32_t ux = static_cast<uint32_t>(x);
    const uint32_t uy = static_cast<uint32_t>(y);
    if (mode == BlendMode::Normal) {
        return fb.blend_pixel(ux, uy, color);
    }

    const auto dest = fb.try_get_pixel(ux, uy);
    if (!dest.has_value()) {
        return false;
    }
    return fb.set_pixel(ux, uy, blend_for_mode(color, *dest, mode));
}

void fill_rect_with_blend(SurfaceRGBA& fb, const RectI& rect, Color color, BlendMode mode) {
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }

    for (int y = rect.y; y < rect.y + rect.height; ++y) {
        for (int x = rect.x; x < rect.x + rect.width; ++x) {
            write_pixel(fb, x, y, color, mode);
        }
    }
}

void draw_line_with_blend(SurfaceRGBA& fb, const LinePrimitive& line, BlendMode mode) {
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
        write_pixel(fb, x0, y0, line.color, mode);
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

} // namespace

RasterizedFrame2D render_draw_list_2d(
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const renderer2d::DrawList2D& draw_list) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = plan.composition.width;
    frame.height = plan.composition.height;
    frame.layer_count = plan.composition.layer_count;
    frame.estimated_draw_ops = draw_list.commands.size();
    frame.backend_name = "cpu-2d-draw-list";
    frame.cache_key = task.cache_key.value;
    frame.note = "Frame rasterized from DrawList2D";
    frame.surface.emplace(static_cast<std::uint32_t>(frame.width), static_cast<std::uint32_t>(frame.height));
    if (!frame.surface.has_value()) {
        return frame;
    }

    frame.surface->clear(renderer2d::Color::transparent());

    const RectI full_clip{0, 0, static_cast<int>(frame.width), static_cast<int>(frame.height)};
    RectI active_clip = full_clip;

    std::vector<const renderer2d::DrawCommand2D*> ordered_commands;
    ordered_commands.reserve(draw_list.commands.size());
    for (const auto& command : draw_list.commands) {
        ordered_commands.push_back(&command);
    }

    if (!ordered_commands.empty()) {
        const bool has_clear = ordered_commands.front()->kind == renderer2d::DrawCommandKind::Clear;
        const auto sort_begin = has_clear ? ordered_commands.begin() + 1 : ordered_commands.begin();
        std::stable_sort(sort_begin, ordered_commands.end(), [](const auto* left, const auto* right) {
            return left->z_order < right->z_order;
        });
    }

    for (const auto* command : ordered_commands) {
        if (command->kind == renderer2d::DrawCommandKind::MaskRect && command->mask_rect.has_value()) {
            active_clip = intersect_rects(active_clip, command->mask_rect->rect);
            frame.surface->set_clip_rect(active_clip);
            continue;
        }

        const RectI effective_clip = command->clip.has_value()
            ? intersect_rects(active_clip, *command->clip)
            : active_clip;
        frame.surface->set_clip_rect(effective_clip);

        switch (command->kind) {
            case renderer2d::DrawCommandKind::Clear:
                if (command->clear.has_value()) {
                    frame.surface->clear(command->clear->color);
                    active_clip = full_clip;
                    frame.surface->set_clip_rect(active_clip);
                }
                break;
            case renderer2d::DrawCommandKind::SolidRect:
                if (command->solid_rect.has_value()) {
                    fill_rect_with_blend(*frame.surface, command->solid_rect->rect, command->solid_rect->color, command->blend_mode);
                }
                break;
            case renderer2d::DrawCommandKind::Line:
                if (command->line.has_value()) {
                    draw_line_with_blend(*frame.surface,
                        renderer2d::LinePrimitive{command->line->x0, command->line->y0,
                                                  command->line->x1, command->line->y1,
                                                  command->line->color},
                        command->blend_mode);
                }
                break;
            case renderer2d::DrawCommandKind::TexturedQuad:
                if (command->textured_quad.has_value()) {
                    const int min_x = static_cast<int>(std::min({command->textured_quad->p0.x, command->textured_quad->p1.x, command->textured_quad->p2.x, command->textured_quad->p3.x}));
                    const int min_y = static_cast<int>(std::min({command->textured_quad->p0.y, command->textured_quad->p1.y, command->textured_quad->p2.y, command->textured_quad->p3.y}));
                    const int max_x = static_cast<int>(std::max({command->textured_quad->p0.x, command->textured_quad->p1.x, command->textured_quad->p2.x, command->textured_quad->p3.x}));
                    const int max_y = static_cast<int>(std::max({command->textured_quad->p0.y, command->textured_quad->p1.y, command->textured_quad->p2.y, command->textured_quad->p3.y}));
                    fill_rect_with_blend(*frame.surface,
                        renderer2d::RectI{min_x, min_y, std::max(1, max_x - min_x), std::max(1, max_y - min_y)},
                        placeholder_textured_color(command->textured_quad->opacity),
                        command->blend_mode);
                }
                break;
            case renderer2d::DrawCommandKind::MaskRect:
                break;
        }
    }

    frame.surface->reset_clip_rect();
    return frame;
}

} // namespace tachyon
