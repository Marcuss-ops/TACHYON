#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/color_transfer.h"
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"

#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

using renderer2d::Color;

namespace {

Color from_spec(const ColorSpec& spec) {
    return Color{
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.r), 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.g), 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.b), 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.a), 0, 255))
    };
}

Color color_with_opacity(Color color, float opacity) {
    color.a = static_cast<std::uint8_t>(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f));
    return color;
}

struct RectI {
    int x, y, width, height;
};

RectI layer_bounds(const scene::EvaluatedLayerState& layer, std::int64_t comp_w, std::int64_t comp_h) {
    int base_width = static_cast<int>(layer.width);
    int base_height = static_cast<int>(layer.height);
    if (base_width <= 0 || base_height <= 0) {
        if (layer.type == scene::LayerType::Text) { base_width = std::max(64, static_cast<int>(comp_w / 6)); base_height = std::max(32, static_cast<int>(comp_h / 10)); }
        else if (layer.type == scene::LayerType::Solid) { base_width = std::max(64, static_cast<int>(comp_w / 4)); base_height = std::max(32, static_cast<int>(comp_h / 8)); }
        else { base_width = 100; base_height = 100; }
    }
    const int w = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * std::abs(layer.local_transform.scale.x))));
    const int h = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * std::abs(layer.local_transform.scale.y))));
    return RectI{ static_cast<int>(std::round(layer.local_transform.position.x)) - w / 2, static_cast<int>(std::round(layer.local_transform.position.y)) - h / 2, w, h };
}

} // namespace

void LayerRenderer::renderLayer(
    const scene::EvaluatedLayerState& layer_state,
    const scene::EvaluatedCompositionState& comp_state,
    RenderContext& context,
    std::vector<float>& accum_r, std::vector<float>& accum_g, std::vector<float>& accum_b, std::vector<float>& accum_a) {

    switch (layer_state.type) {
    case scene::LayerType::Solid: renderSolidLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    case scene::LayerType::Image: renderImageLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    case scene::LayerType::Text: renderTextLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    case scene::LayerType::Shape: renderShapeLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    default: break;
    }
}

void LayerRenderer::renderSolidLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)layer; (void)comp; (void)context; (void)r; (void)g; (void)b; (void)a;
    // Basic solid fill logic would go here, updating accumulation buffers
}

void LayerRenderer::renderShapeLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)comp; (void)context; (void)layer; (void)r; (void)g; (void)b; (void)a;
}

void LayerRenderer::renderImageLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)comp; (void)context; (void)layer; (void)r; (void)g; (void)b; (void)a;
}

void LayerRenderer::renderTextLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)comp; (void)context; (void)layer; (void)r; (void)g; (void)b; (void)a;
}

} // namespace tachyon::renderer2d
