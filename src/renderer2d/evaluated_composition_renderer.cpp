#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include <algorithm>
#include <cmath>

namespace tachyon {
namespace {

renderer2d::Color color_with_opacity(renderer2d::Color base, float opacity) {
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    base.a = static_cast<std::uint8_t>(std::round(255.0f * clamped));
    base.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(base.r) * base.a + 127U) / 255U);
    base.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(base.g) * base.a + 127U) / 255U);
    base.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(base.b) * base.a + 127U) / 255U);
    return base;
}

renderer2d::Color layer_debug_color(const timeline::EvaluatedLayerState& layer) {
    using timeline::LayerType;
    switch (layer.type) {
        case LayerType::Solid: return renderer2d::Color{64, 128, 255, 255};
        case LayerType::Text: return renderer2d::Color{255, 255, 255, 255};
        case LayerType::Image: return renderer2d::Color{255, 180, 64, 255};
        case LayerType::Camera: return renderer2d::Color{0, 0, 0, 0};
        default: return renderer2d::Color{160, 160, 160, 255};
    }
}

renderer2d::RectPrimitive layer_rect(const timeline::EvaluatedCompositionState& state, const timeline::EvaluatedLayerState& layer) {
    const int base_w = std::max(64, state.width / 4);
    const int base_h = std::max(32, state.height / 8);
    const int w = std::max(1, static_cast<int>(std::round(base_w * layer.transform2.scale.x)));
    const int h = std::max(1, static_cast<int>(std::round(base_h * layer.transform2.scale.y)));
    const int x = static_cast<int>(std::round(layer.transform2.position.x));
    const int y = static_cast<int>(std::round(layer.transform2.position.y));
    return renderer2d::RectPrimitive{x, y, w, h, color_with_opacity(layer_debug_color(layer), layer.opacity)};
}

} // namespace

std::vector<renderer2d::DrawCommand2D> build_draw_commands_from_evaluated_state(
    const timeline::EvaluatedCompositionState& state) {

    std::vector<renderer2d::DrawCommand2D> commands;
    commands.reserve(state.layers.size() + 1);
    commands.push_back(renderer2d::DrawCommand2D::make_clear(renderer2d::Color::transparent()));

    std::vector<const timeline::EvaluatedLayerState*> ordered;
    ordered.reserve(state.layers.size());
    for (const auto& layer : state.layers) {
        ordered.push_back(&layer);
    }

    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) {
        return a->z_order < b->z_order;
    });

    for (const timeline::EvaluatedLayerState* layer : ordered) {
        if (!layer->visible || layer->type == timeline::LayerType::Camera) {
            continue;
        }
        commands.push_back(renderer2d::DrawCommand2D::make_rect(layer_rect(state, *layer)));
    }

    return commands;
}

RasterizedFrame2D render_evaluated_composition_2d(
    const timeline::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    const auto commands = build_draw_commands_from_evaluated_state(state);
    return render_frame_2d(plan, task, commands);
}

} // namespace tachyon
