#include "tachyon/renderer2d/draw_command.h"
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

DrawCommand2D solid_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    DrawCommand2D command;
    command.kind = DrawCommandKind::SolidRect;
    command.z_order = z_order;
    command.blend_mode = BlendMode::Normal;
    command.clip = full_clip(composition_state);
    command.solid_rect = SolidRectCommand{scaled_rect(layer, 100, 100), color_with_opacity(static_cast<float>(layer.opacity)), static_cast<float>(layer.opacity)};
    return command;
}

DrawCommand2D image_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order, const char* prefix, int base_width, int base_height) {
    const RectI rect = scaled_rect(layer, base_width, base_height);
    TexturedQuadCommand quad;
    quad.texture = TextureHandle{std::string(prefix) + layer.id};
    quad.p0 = {static_cast<float>(rect.x), static_cast<float>(rect.y)};
    quad.p1 = {static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y)};
    quad.p2 = {static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y + rect.height)};
    quad.p3 = {static_cast<float>(rect.x), static_cast<float>(rect.y + rect.height)};
    quad.opacity = static_cast<float>(layer.opacity);

    DrawCommand2D command;
    command.kind = DrawCommandKind::TexturedQuad;
    command.z_order = z_order;
    command.blend_mode = BlendMode::Normal;
    command.clip = full_clip(composition_state);
    command.textured_quad = quad;
    return command;
}

} // namespace

std::vector<DrawCommand2D> map_layer_to_draw_commands(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    std::vector<DrawCommand2D> commands;
    if (!layer.enabled || !layer.active || layer.is_camera) {
        return commands;
    }
    if (layer.type == "solid") {
        commands.push_back(solid_command(layer, composition_state, z_order));
    } else if (layer.type == "image") {
        commands.push_back(image_command(layer, composition_state, z_order, "image:", 256, 256));
    } else if (layer.type == "text") {
        commands.push_back(image_command(layer, composition_state, z_order, "text:", 256, 64));
    }
    return commands;
}

} // namespace renderer2d
} // namespace tachyon
