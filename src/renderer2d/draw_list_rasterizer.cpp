#include "tachyon/renderer2d/draw_list_rasterizer.h"

#include "tachyon/renderer2d/rasterizer_ops.h"

#include <algorithm>
#include <vector>

namespace tachyon {
namespace {

renderer2d::Color placeholder_textured_color(float opacity) {
    renderer2d::Color color{255, 255, 255, 255};
    color.a = static_cast<std::uint8_t>(opacity <= 0.0f ? 0.0f : (opacity >= 1.0f ? 255.0f : opacity * 255.0f));
    color.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.r) * color.a + 127U) / 255U);
    color.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.g) * color.a + 127U) / 255U);
    color.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.b) * color.a + 127U) / 255U);
    return color;
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
        if (command->clip.has_value()) {
            frame.surface->set_clip_rect(*command->clip);
        } else {
            frame.surface->reset_clip_rect();
        }

        switch (command->kind) {
            case renderer2d::DrawCommandKind::Clear:
                if (command->clear.has_value()) {
                    frame.surface->clear(command->clear->color);
                }
                break;
            case renderer2d::DrawCommandKind::SolidRect:
                if (command->solid_rect.has_value()) {
                    renderer2d::CPURasterizer::draw_rect(*frame.surface,
                        renderer2d::RectPrimitive{command->solid_rect->rect.x, command->solid_rect->rect.y,
                                                  command->solid_rect->rect.width, command->solid_rect->rect.height,
                                                  command->solid_rect->color});
                }
                break;
            case renderer2d::DrawCommandKind::Line:
                if (command->line.has_value()) {
                    renderer2d::CPURasterizer::draw_line(*frame.surface,
                        renderer2d::LinePrimitive{command->line->x0, command->line->y0,
                                                  command->line->x1, command->line->y1,
                                                  command->line->color});
                }
                break;
            case renderer2d::DrawCommandKind::TexturedQuad:
                if (command->textured_quad.has_value()) {
                    const int min_x = static_cast<int>(std::min({command->textured_quad->p0.x, command->textured_quad->p1.x, command->textured_quad->p2.x, command->textured_quad->p3.x}));
                    const int min_y = static_cast<int>(std::min({command->textured_quad->p0.y, command->textured_quad->p1.y, command->textured_quad->p2.y, command->textured_quad->p3.y}));
                    const int max_x = static_cast<int>(std::max({command->textured_quad->p0.x, command->textured_quad->p1.x, command->textured_quad->p2.x, command->textured_quad->p3.x}));
                    const int max_y = static_cast<int>(std::max({command->textured_quad->p0.y, command->textured_quad->p1.y, command->textured_quad->p2.y, command->textured_quad->p3.y}));
                    renderer2d::CPURasterizer::draw_rect(*frame.surface,
                        renderer2d::RectPrimitive{min_x, min_y, std::max(1, max_x - min_x), std::max(1, max_y - min_y),
                                                  placeholder_textured_color(command->textured_quad->opacity)});
                }
                break;
        }
    }

    frame.surface->reset_clip_rect();
    return frame;
}

} // namespace tachyon
