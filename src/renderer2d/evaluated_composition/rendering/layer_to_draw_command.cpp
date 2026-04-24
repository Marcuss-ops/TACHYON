#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/renderer2d/spec/project_card.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <algorithm>
#include <cmath>
#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/renderer2d/spec/project_card.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon {
namespace renderer2d {
namespace {

RectI full_clip(const scene::EvaluatedCompositionState& composition_state) {
    return RectI{0, 0, static_cast<int>(composition_state.width), static_cast<int>(composition_state.height)};
}

Color map_color(const ColorSpec& spec) {
    return Color{
        static_cast<float>(spec.r) / 255.0f,
        static_cast<float>(spec.g) / 255.0f,
        static_cast<float>(spec.b) / 255.0f,
        static_cast<float>(spec.a) / 255.0f
    };
}

BlendMode map_blend_mode(const std::string& mode) {
    if (mode == "additive") return BlendMode::Additive;
    if (mode == "multiply") return BlendMode::Multiply;
    if (mode == "screen") return BlendMode::Screen;
    if (mode == "overlay") return BlendMode::Overlay;
    if (mode == "soft_light" || mode == "softLight") return BlendMode::SoftLight;
    return BlendMode::Normal;
}

RectI scaled_rect(const scene::EvaluatedLayerState& layer, int base_width, int base_height) {
    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * layer.local_transform.scale.x)));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * layer.local_transform.scale.y)));
    return RectI{
        static_cast<int>(std::round(layer.local_transform.position.x)),
        static_cast<int>(std::round(layer.local_transform.position.y)),
        width, height};
}

Color color_with_opacity(const ColorSpec& spec, float opacity) {
    Color color = map_color(spec);
    color.a *= std::clamp(opacity, 0.0f, 1.0f);
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
    card.rotation_degrees = 0.0f; // Extracting from quaternion is slow, using 0 for flat cards
    card.opacity = static_cast<float>(layer.opacity);
    return card;
}

DrawCommand2D solid_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    const int base_width = layer.width > 0 ? layer.width : static_cast<int>(composition_state.width);
    const int base_height = layer.height > 0 ? layer.height : static_cast<int>(composition_state.height);
    const RectI rect = scaled_rect(layer, base_width, base_height);
    const Color color = color_with_opacity(layer.fill_color, static_cast<float>(layer.opacity));
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

DrawCommand2D shape_command(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    DrawCommand2D command;
    command.kind = DrawCommandKind::Shape;
    command.z_order = z_order;
    command.blend_mode = map_blend_mode(layer.blend_mode);
    command.clip = full_clip(composition_state);
    
    ShapeCommand shape;
    shape.fill_color = map_color(layer.fill_color);
    shape.stroke_color = map_color(layer.stroke_color);
    shape.stroke_width = layer.stroke_width;
    shape.line_cap = layer.line_cap;
    shape.line_join = layer.line_join;
    shape.miter_limit = layer.miter_limit;
    shape.opacity = static_cast<float>(layer.opacity);
    shape.transform = layer.local_transform;
    
    if (layer.shape_path.has_value()) {
        const auto& points = layer.shape_path->points;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const auto& pt = points[i];
            if (i == 0) {
                shape.geometry.commands.push_back({PathVerb::MoveTo, pt.position});
            } else {
                const auto& prev = points[i-1];
                if ((prev.tangent_out.x == 0.0f && prev.tangent_out.y == 0.0f) && 
                    (pt.tangent_in.x == 0.0f && pt.tangent_in.y == 0.0f)) {
                    shape.geometry.commands.push_back({PathVerb::LineTo, pt.position});
                } else {
                    shape.geometry.commands.push_back({
                        PathVerb::CubicTo, 
                        prev.position + prev.tangent_out,
                        pt.position + pt.tangent_in,
                        pt.position
                    });
                }
            }
        }
        if (layer.shape_path->closed && !points.empty()) {
             const auto& last = points.back();
             const auto& first = points.front();
             if ((last.tangent_out.x != 0.0f || last.tangent_out.y != 0.0f) || 
                 (first.tangent_in.x != 0.0f || first.tangent_in.y != 0.0f)) {
                 shape.geometry.commands.push_back({
                     PathVerb::CubicTo,
                     last.position + last.tangent_out,
                     first.position + first.tangent_in,
                     first.position
                 });
             }
             shape.geometry.commands.push_back({PathVerb::Close});
        }
    }
    
    command.shape.emplace(std::move(shape));
    return command;
}

} // namespace

DrawList2D generate_draw_list(const scene::EvaluatedCompositionState& state) {
    DrawList2D list;
    int z_order = 0;
    
    for (const auto& layer : state.layers) {
        if (!layer.visible) {
            z_order++;
            continue;
        }

        switch (layer.type) {
            case scene::LayerType::Solid:
                list.commands.push_back(solid_command(layer, state, z_order));
                break;
            case scene::LayerType::Mask:
                list.commands.push_back(mask_command(layer, state, z_order));
                break;
            case scene::LayerType::Image:
            case scene::LayerType::Video:
                list.commands.push_back(image_command(layer, state, z_order, "asset:", static_cast<int>(layer.width), static_cast<int>(layer.height)));
                break;
            case scene::LayerType::Shape:
                list.commands.push_back(shape_command(layer, state, z_order));
                break;
            default:
                break;
        }
        z_order++;
    }
    
    return list;
}

std::vector<DrawCommand2D> map_layer_to_draw_commands(const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& composition_state, int z_order) {
    std::vector<DrawCommand2D> commands;
    if (!layer.enabled || !layer.active || layer.type == scene::LayerType::Camera) {
        return commands;
    }
    
    using scene::LayerType;
    
    if (layer.is_adjustment_layer) {
        DrawCommand2D adjustment;
        adjustment.kind = DrawCommandKind::Adjustment;
        adjustment.z_order = z_order;
        adjustment.blend_mode = map_blend_mode(layer.blend_mode);
        adjustment.clip = full_clip(composition_state);
        
        AdjustmentCommand adj_cmd;
        adj_cmd.layer_id = layer.id;
        for (const auto& effect : layer.effects) {
            EffectParams params;
            for (const auto& [k, v] : effect.scalars) {
                params.scalars[k] = static_cast<float>(v);
            }
            for (const auto& [k, v] : effect.colors) {
                params.colors[k] = Color{
                    static_cast<float>(v.r) / 255.0f,
                    static_cast<float>(v.g) / 255.0f,
                    static_cast<float>(v.b) / 255.0f,
                    static_cast<float>(v.a) / 255.0f
                };
            }
            for (const auto& [k, v] : effect.strings) {
                params.strings[k] = v;
            }
            adj_cmd.effects.push_back({effect.type, std::move(params)});
        }
        adjustment.adjustment.emplace(std::move(adj_cmd));
        commands.push_back(std::move(adjustment));
        return commands;
    }

    if (layer.type == LayerType::Solid) {
        commands.push_back(solid_command(layer, composition_state, z_order));
    } else if (layer.type == LayerType::Shape) {
        commands.push_back(shape_command(layer, composition_state, z_order));
    } else if (layer.type == LayerType::Mask) {
        commands.push_back(mask_command(layer, composition_state, z_order));
    } else if (layer.type == LayerType::Image) {
        auto command = image_command(layer, composition_state, z_order, "image:", 256, 256);
        if (command.textured_quad.has_value()) {
            commands.push_back(std::move(command));
        }
    } else if (layer.type == LayerType::Text) {
        auto command = image_command(layer, composition_state, z_order, "text:", 256, 64);
        if (command.textured_quad.has_value()) {
            commands.push_back(std::move(command));
        }
    }
    return commands;
}

} // namespace renderer2d

std::vector<renderer2d::DrawCommand2D> build_draw_commands_from_evaluated_state(
    const scene::EvaluatedCompositionState& state) {
    return renderer2d::generate_draw_list(state).commands;
}

} // namespace tachyon
