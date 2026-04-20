#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"

#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

namespace {

RectI layer_rect(const scene::EvaluatedLayerState& layer, std::int64_t comp_width, std::int64_t comp_height) {
    const std::int64_t base_width = layer.width > 0 ? layer.width : comp_width;
    const std::int64_t base_height = layer.height > 0 ? layer.height : comp_height;
    const float scale_x = std::max(0.0f, std::abs(layer.local_transform.scale.x));
    const float scale_y = std::max(0.0f, std::abs(layer.local_transform.scale.y));
    const int width = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_width) * static_cast<double>(scale_x))));
    const int height = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_height) * static_cast<double>(scale_y))));
    return RectI{
        static_cast<int>(std::lround(layer.local_transform.position.x)),
        static_cast<int>(std::lround(layer.local_transform.position.y)),
        width,
        height
    };
}

RectI shape_bounds_from_path(const scene::EvaluatedShapePath& path) {
    if (path.points.empty()) {
        return RectI{0, 0, 0, 0};
    }

    float min_x = path.points.front().position.x;
    float min_y = path.points.front().position.y;
    float max_x = path.points.front().position.x;
    float max_y = path.points.front().position.y;
    for (const auto& point : path.points) {
        min_x = std::min(min_x, point.position.x);
        min_y = std::min(min_y, point.position.y);
        max_x = std::max(max_x, point.position.x);
        max_y = std::max(max_y, point.position.y);
    }

    return RectI{
        static_cast<int>(std::floor(min_x)),
        static_cast<int>(std::floor(min_y)),
        std::max(1, static_cast<int>(std::ceil(max_x - min_x))),
        std::max(1, static_cast<int>(std::ceil(max_y - min_y)))
    };
}

void fill_rect_alpha(
    std::vector<float>& accum_a,
    std::int64_t width,
    std::int64_t height,
    const RectI& rect,
    float value) {

    if (rect.width <= 0 || rect.height <= 0 || width <= 0 || height <= 0) {
        return;
    }

    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(static_cast<int>(width), rect.x + rect.width);
    const int y1 = std::min(static_cast<int>(height), rect.y + rect.height);
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
            if (index < accum_a.size()) {
                accum_a[index] = std::clamp(value, 0.0f, 1.0f);
            }
        }
    }
}

} // namespace

void MaskRenderer::applyMask(
    const scene::EvaluatedCompositionState& state,
    RenderContext& context,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a) {
    (void)context;
    const std::size_t expected_size = static_cast<std::size_t>(std::max<std::int64_t>(1, state.width)) *
                                      static_cast<std::size_t>(std::max<std::int64_t>(1, state.height));
    if (accum_a.size() < expected_size || accum_r.size() < expected_size || accum_g.size() < expected_size || accum_b.size() < expected_size) {
        return;
    }

    std::vector<float> mask_alpha(expected_size, 1.0f);
    bool found_mask = false;
    for (const auto& layer : state.layers) {
        if (layer.type != scene::LayerType::Mask || !layer.enabled || !layer.active) {
            continue;
        }
        found_mask = true;
        const RectI rect = layer.shape_path.has_value()
            ? shape_bounds_from_path(*layer.shape_path)
            : layer_rect(layer, state.width, state.height);
        fill_rect_alpha(mask_alpha, state.width, state.height, rect, static_cast<float>(layer.opacity));
    }

    if (!found_mask) {
        return;
    }

    for (std::size_t index = 0; index < expected_size; ++index) {
        const float factor = std::clamp(mask_alpha[index], 0.0f, 1.0f);
        accum_r[index] *= factor;
        accum_g[index] *= factor;
        accum_b[index] *= factor;
        accum_a[index] *= factor;
    }
}

void MaskRenderer::computeMaskBounds(
    const scene::EvaluatedLayerState& layer_state,
    int& x0, int& y0, int& x1, int& y1) {
    (void)layer_state;
    x0 = y0 = x1 = y1 = 0;
}

} // namespace tachyon::renderer2d
