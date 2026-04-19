#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/renderer2d/project_card.h"
#include "tachyon/scene/evaluated_state.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon {
namespace renderer2d {
namespace {

RectI full_clip(const scene::EvaluatedCompositionState& composition_state) {
    return RectI{0, 0, static_cast<int>(composition_state.width), static_cast<int>(composition_state.height)};
}

RectI scaled_rect(const scene::EvaluatedLayerState& layer, int base_width, int base_height) {
    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * layer.scale.x)));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * layer.scale.y)));
    return RectI{static_cast<int>(std::round(layer.position.x)), static_cast<int>(std::round(layer.position.y)), width, height};
}

Color color_with_opacity(float opacity) {
    Color color = Color::white();
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    color.a = static_cast<uint8_t>(std::round(255.0f * clamped));
    return color;
}

bool is_camera_aware_card(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state) {
    return composition_state.camera.available && (layer.type == "image" || layer.type == "text");
}

float camera_card_depth(const scene::EvaluatedLayerState& layer, int z_order) {
    if (layer.type == "text") {
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
        layer.position.x,
        layer.position.y,
        camera_card_depth(layer, z_order)
    };
    card.width = static_cast<float>(base_width) * std::max(0.1f, layer.scale.x);
    card.height = static_cast<float>(base_height) * std::max(0.1f, layer.scale.y);
    card.rotation_degrees = static_cast<float>(layer.rotation_degrees);
    card.opacity = static_cast<float>(layer.opacity);
    return card;
}

DrawCommand2D solid_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    const RectI rect = scaled_rect(layer, 100, 100);
    const Color color = color_with_opacity(static_cast<float>(layer.opacity));
    DrawCommand2D command;
    command.kind = DrawCommandKind::SolidRect;
    command.z_order = z_order;
    command.blend_mode = BlendMode::Normal;
    command.clip = full_clip(composition_state);
    command.solid_rect.emplace(SolidRectCommand{rect, color, static_cast<float>(layer.opacity)});
    return command;
}

DrawCommand2D mask_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    DrawCommand2D command;
    command.kind = DrawCommandKind::MaskRect;
    command.z_order = z_order;
    command.blend_mode = BlendMode::Normal;
    command.clip = full_clip(composition_state);
    command.mask_rect.emplace(MaskRectCommand{scaled_rect(layer, 100, 100)});
    return command;
}

DrawCommand2D image_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order, const char* prefix, int base_width, int base_height) {
    DrawCommand2D command;
    command.kind = DrawCommandKind::TexturedQuad;
    command.blend_mode = BlendMode::Normal;
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

std::vector<DrawCommand2D> map_layer_to_draw_commands(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    std::vector<DrawCommand2D> commands;
    if (!layer.enabled || !layer.active || layer.is_camera) {
        return commands;
    }
    if (layer.type == "solid" || layer.type == "shape") {
        commands.push_back(solid_command(layer, composition_state, z_order));
    } else if (layer.type == "mask") {
        commands.push_back(mask_command(layer, composition_state, z_order));
    } else if (layer.type == "image") {
        auto command = image_command(layer, composition_state, z_order, "image:", 256, 256);
        if (command.textured_quad.has_value()) {
            commands.push_back(std::move(command));
        }
    } else if (layer.type == "text") {
        auto command = image_command(layer, composition_state, z_order, "text:", 256, 64);
        if (command.textured_quad.has_value()) {
            commands.push_back(std::move(command));
        }
    }
    return commands;
}

} // namespace renderer2d
} // namespace tachyon
