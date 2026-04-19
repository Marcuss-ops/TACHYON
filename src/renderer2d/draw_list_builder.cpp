#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/project_card.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace tachyon {
namespace renderer2d {
namespace {

timeline::EvaluatedCompositionState to_timeline_state(const scene::EvaluatedCompositionState& composition_state) {
    timeline::EvaluatedCompositionState converted;
    converted.composition_id = composition_state.composition_id;
    converted.width = static_cast<int>(composition_state.width);
    converted.height = static_cast<int>(composition_state.height);
    converted.time_seconds = composition_state.composition_time_seconds;
    converted.layers.reserve(composition_state.layers.size());

    for (const auto& layer : composition_state.layers) {
        timeline::EvaluatedLayerState converted_layer;
        converted_layer.id = layer.id;
        converted_layer.blend_mode = layer.blend_mode;
        converted_layer.visible = layer.visible;
        converted_layer.opacity = static_cast<float>(layer.opacity);
        converted_layer.transform2.position = layer.position;
        converted_layer.transform2.scale = layer.scale;
        if (layer.type == "solid") {
            converted_layer.type = timeline::LayerType::Solid;
        } else if (layer.type == "shape") {
            converted_layer.type = timeline::LayerType::Shape;
        } else if (layer.type == "mask") {
            converted_layer.type = timeline::LayerType::Mask;
        } else if (layer.type == "image") {
            converted_layer.type = timeline::LayerType::Image;
        } else if (layer.type == "text") {
            converted_layer.type = timeline::LayerType::Text;
        } else if (layer.is_camera) {
            converted_layer.type = timeline::LayerType::Camera;
        }
        converted.layers.push_back(converted_layer);
    }

    return converted;
}

RectI full_clip(const timeline::EvaluatedCompositionState& composition_state) {
    return RectI{0, 0, static_cast<int>(composition_state.width), static_cast<int>(composition_state.height)};
}

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
    return BlendMode::Normal;
}

RectI scaled_rect(const timeline::EvaluatedLayerState& layer, int base_width, int base_height) {
    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * layer.transform2.scale.x)));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * layer.transform2.scale.y)));
    return RectI{
        static_cast<int>(std::round(layer.transform2.position.x)),
        static_cast<int>(std::round(layer.transform2.position.y)),
        width,
        height
    };
}

RectI scaled_rect(const scene::EvaluatedLayerState& layer, int base_width, int base_height) {
    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * static_cast<float>(layer.scale.x))));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * static_cast<float>(layer.scale.y))));
    return RectI{
        static_cast<int>(std::round(layer.position.x)),
        static_cast<int>(std::round(layer.position.y)),
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
    card.width = static_cast<float>(base_width) * std::max(0.1f, static_cast<float>(layer.scale.x));
    card.height = static_cast<float>(base_height) * std::max(0.1f, static_cast<float>(layer.scale.y));
    card.rotation_degrees = static_cast<float>(layer.rotation_degrees);
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

DrawCommand2D mask_command(const timeline::EvaluatedLayerState& layer, const timeline::EvaluatedCompositionState& composition_state, int z_order) {
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

DrawList2D DrawListBuilder::build(const timeline::EvaluatedCompositionState& composition_state) {
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
        if (!layer.visible || layer.type == timeline::LayerType::Camera) {
            continue;
        }

    if (layer.type == timeline::LayerType::Solid || layer.type == timeline::LayerType::Shape) {
        draw_list.commands.emplace_back();
        DrawCommand2D& command = draw_list.commands.back();
        command.kind = DrawCommandKind::SolidRect;
        command.z_order = static_cast<int>(index);
        command.blend_mode = parse_blend_mode(layer.blend_mode);
            command.clip = full_clip(composition_state);
            command.solid_rect.emplace(SolidRectCommand{
                scaled_rect(layer, 100, 100),
                Color::white(),
                static_cast<float>(layer.opacity)
            });
            continue;
        }

        if (layer.type == timeline::LayerType::Mask) {
            draw_list.commands.push_back(mask_command(layer, composition_state, static_cast<int>(index)));
            continue;
        }

        if (layer.type == timeline::LayerType::Image || layer.type == timeline::LayerType::Text) {
            draw_list.commands.emplace_back();
            DrawCommand2D& command = draw_list.commands.back();
            command.kind = DrawCommandKind::TexturedQuad;
            command.z_order = static_cast<int>(index);
            command.blend_mode = parse_blend_mode(layer.blend_mode);
            command.clip = full_clip(composition_state);

            TexturedQuadCommand quad;
            quad.texture = TextureHandle{std::string(layer.type == timeline::LayerType::Image ? "image:" : "text:") + layer.id};
            const RectI rect = scaled_rect(layer, 256, layer.type == timeline::LayerType::Image ? 256 : 64);
            quad.p0 = {static_cast<float>(rect.x), static_cast<float>(rect.y)};
            quad.p1 = {static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y)};
            quad.p2 = {static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y + rect.height)};
            quad.p3 = {static_cast<float>(rect.x), static_cast<float>(rect.y + rect.height)};
            quad.opacity = static_cast<float>(layer.opacity);
            command.textured_quad.emplace(std::move(quad));
        }
    }

    return draw_list;
}

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
        if (!layer.enabled || !layer.active || layer.is_camera) {
            continue;
        }

        if (layer.type == "solid" || layer.type == "shape") {
            draw_list.commands.push_back(solid_command(layer, composition_state, static_cast<int>(index)));
        } else if (layer.type == "mask") {
            draw_list.commands.push_back(mask_command(layer, composition_state, static_cast<int>(index)));
        } else if (layer.type == "image") {
            auto command = image_command(layer, composition_state, static_cast<int>(index), "image:", 256, 256);
            if (command.textured_quad.has_value()) {
                draw_list.commands.push_back(std::move(command));
            }
        } else if (layer.type == "text") {
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
