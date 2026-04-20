#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/project_card.h"

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

bool is_camera_aware_card(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state) {
    return composition_state.camera.available && (layer.type == scene::LayerType::Image || layer.type == scene::LayerType::Text);
}

float camera_card_depth(const scene::EvaluatedLayerState& layer, int z_order) {
    if (layer.type == scene::LayerType::Text) {
        return -120.0f - static_cast<float>(z_order * 20);
    }
    return -360.0f - static_cast<float>(z_order * 40);
}

RenderableCard3D make_camera_card(
    const scene::EvaluatedLayerState& layer,
    int z_order,
    const char* prefix,
    int base_width,
    int base_height) {

    RenderableCard3D card;
    card.texture = TextureHandle{std::string(prefix) + layer.id};
    card.center_world = {
        layer.world_position3.x,
        layer.world_position3.y,
        camera_card_depth(layer, z_order)
    };
    card.width = static_cast<float>(base_width) * std::max(0.1f, layer.local_transform.scale.x);
    card.height = static_cast<float>(base_height) * std::max(0.1f, layer.local_transform.scale.y);
    card.rotation_degrees = 0.0f; // Simplified
    card.opacity = static_cast<float>(layer.opacity);
    return card;
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
    command.kind = DrawCommandKind::TexturedQuad;
    command.blend_mode = parse_blend_mode(layer.blend_mode);
    command.clip = full_clip(composition_state);

    if (is_camera_aware_card(layer, composition_state)) {
        const RenderableCard3D card = make_camera_card(layer, z_order, prefix, base_width, base_height);
        const ProjectedCard3D projected = project_card_to_screen(
            card,
            composition_state.camera.camera,
            static_cast<float>(composition_state.width),
            static_cast<float>(composition_state.height));
        if (!projected.visible) {
            return DrawCommand2D{};
        }
        command.z_order = projected.depth_sort_key;
        command.textured_quad = projected.quad;
        return command;
    }

    const RectI rect = scaled_rect(layer, base_width, base_height);
    TexturedQuadCommand quad;
    quad.texture = TextureHandle{std::string(prefix) + layer.id};
    quad.p0 = {static_cast<float>(rect.x), static_cast<float>(rect.y)};
    quad.p1 = {static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y)};
    quad.p2 = {static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y + rect.height)};
    quad.p3 = {static_cast<float>(rect.x), static_cast<float>(rect.y + rect.height)};
    quad.opacity = static_cast<float>(layer.opacity);

    command.z_order = z_order;
    command.textured_quad = quad;
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
        if (!layer.enabled || !layer.active || layer.type == scene::LayerType::Camera) {
            continue;
        }

        using scene::LayerType;

        if (layer.type == LayerType::Solid || layer.type == LayerType::Shape) {
            draw_list.commands.push_back(solid_command(layer, composition_state, static_cast<int>(index)));
        } else if (layer.type == LayerType::Mask) {
            draw_list.commands.push_back(mask_command(layer, composition_state, static_cast<int>(index)));
        } else if (layer.type == LayerType::Image) {
            auto command = image_command(layer, composition_state, static_cast<int>(index), "image:", 256, 256);
            if (command.textured_quad.has_value()) {
                draw_list.commands.push_back(std::move(command));
            }
        } else if (layer.type == LayerType::Text) {
            auto command = image_command(layer, composition_state, static_cast<int>(index), "text:", 256, 64);
            if (command.textured_quad.has_value()) {
                draw_list.commands.push_back(std::move(command));
            }
        }
    }

    return draw_list;
}

} // namespace renderer2d
} // namespace tachyon
