#include "tachyon/renderer2d/raster/draw_list_builder.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace tachyon {
namespace renderer2d {
namespace {

RectI full_clip(const scene::EvaluatedCompositionState& composition_state) {
    return RectI{0, 0, static_cast<int>(composition_state.width), static_cast<int>(composition_state.height)};
}

BlendMode parse_blend_mode(const std::string& blend_mode) {
    if (blend_mode == "additive") {
        return BlendMode::Additive;
    }
    if (blend_mode == "multiply") {
        return BlendMode::Multiply;
    }
    if (blend_mode == "screen") {
        return BlendMode::Screen;
    }
    if (blend_mode == "overlay") {
        return BlendMode::Overlay;
    }
    if (blend_mode == "soft_light" || blend_mode == "softLight") {
        return BlendMode::SoftLight;
    }
    return BlendMode::Normal;
}

RectI scaled_rect(const scene::EvaluatedLayerState& layer, int base_width, int base_height) {
    // Note: We use the world_matrix now for more accuracy, but preserving the old logic for fallback
    // In a future pass, we'll replace this with proper quad projection
    const float scale_x = layer.local_transform.scale.x;
    const float scale_y = layer.local_transform.scale.y;
    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * scale_x)));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * scale_y)));
    return RectI{
        static_cast<int>(std::round(layer.local_transform.position.x)),
        static_cast<int>(std::round(layer.local_transform.position.y)),
        width,
        height
    };
}

Color color_with_opacity(float opacity) {
    Color color = Color::white();
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    color.a = static_cast<uint8_t>(std::round(255.0f * clamped));
    return color;
}


DrawCommand2D solid_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    const RectI rect = scaled_rect(layer, 100, 100);
    DrawCommand2D command;
    command.kind = DrawCommandKind::SolidRect;
    command.z_order = z_order;
    command.blend_mode = parse_blend_mode(layer.blend_mode);
    command.clip = full_clip(composition_state);
    command.solid_rect.emplace(SolidRectCommand{rect, color_with_opacity(static_cast<float>(layer.opacity)), static_cast<float>(layer.opacity)});
    return command;
}

DrawCommand2D mask_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    DrawCommand2D command;
    command.kind = DrawCommandKind::MaskRect;
    command.z_order = z_order;
    command.blend_mode = parse_blend_mode(layer.blend_mode);
    command.clip = full_clip(composition_state);
    command.mask_rect.emplace(MaskRectCommand{scaled_rect(layer, 100, 100)});
    return command;
}

DrawCommand2D image_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order, const char* prefix, int base_width, int base_height) {
    DrawCommand2D command;
    command.kind = DrawCommandKind::TexturedRect;
    command.blend_mode = parse_blend_mode(layer.blend_mode);
    command.clip = full_clip(composition_state);

    const RectI rect = scaled_rect(layer, base_width, base_height);
    TexturedRectCommand tex_rect;
    tex_rect.texture = TextureHandle{std::string(prefix) + layer.id};
    tex_rect.rect = rect;
    tex_rect.opacity = static_cast<float>(layer.opacity);

    command.z_order = z_order;
    command.textured_rect = tex_rect;
    return command;
}

} // namespace

DrawList2D DrawListBuilder::build(const scene::EvaluatedCompositionState& composition_state) {
    DrawList2D draw_list;
    draw_list.commands.reserve(composition_state.layers.size() + 1);

    DrawCommand2D clear_command;
    clear_command.kind = DrawCommandKind::Clear;
    clear_command.z_order = -1;
    clear_command.blend_mode = BlendMode::Normal;
    clear_command.clear = ClearCommand{Color::black()};
    draw_list.commands.push_back(clear_command);

    for (std::size_t index = 0; index < composition_state.layers.size(); ++index) {
        const auto& layer = composition_state.layers[index];
        if (!layer.enabled || !layer.active) {
            continue;
        }

        if (layer.type == LayerType::Solid || layer.type == LayerType::Shape) {
            draw_list.commands.push_back(solid_command(layer, composition_state, static_cast<int>(index)));
        } else if (layer.type == LayerType::Mask) {
            draw_list.commands.push_back(mask_command(layer, composition_state, static_cast<int>(index)));
        } else if (layer.type == LayerType::Image) {
            auto command = image_command(layer, composition_state, static_cast<int>(index), "image:", 256, 256);
            if (command.textured_rect.has_value()) {
                draw_list.commands.push_back(std::move(command));
            }
        } else if (layer.type == LayerType::Text) {
            auto command = image_command(layer, composition_state, static_cast<int>(index), "text:", 256, 64);
            if (command.textured_rect.has_value()) {
                draw_list.commands.push_back(std::move(command));
            }
        } else if (layer.type == LayerType::Procedural) {
            draw_list.commands.push_back(solid_command(layer, composition_state, static_cast<int>(index)));
        }
    }

    return draw_list;
}

} // namespace renderer2d
} // namespace tachyon
