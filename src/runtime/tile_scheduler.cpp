#include "tachyon/runtime/tile_scheduler.h"

#include <algorithm>
#include <cmath>

namespace tachyon {
namespace {

renderer2d::RectI union_rects(const renderer2d::RectI& a, const renderer2d::RectI& b) {
    if (a.width <= 0 || a.height <= 0) {
        return b;
    }
    if (b.width <= 0 || b.height <= 0) {
        return a;
    }

    const int x0 = std::min(a.x, b.x);
    const int y0 = std::min(a.y, b.y);
    const int x1 = std::max(a.x + a.width, b.x + b.width);
    const int y1 = std::max(a.y + a.height, b.y + b.height);
    return renderer2d::RectI{x0, y0, x1 - x0, y1 - y0};
}

renderer2d::RectI clamp_to_frame(const renderer2d::RectI& rect, std::int64_t width, std::int64_t height) {
    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(static_cast<int>(width), rect.x + rect.width);
    const int y1 = std::min(static_cast<int>(height), rect.y + rect.height);
    if (x1 <= x0 || y1 <= y0) {
        return renderer2d::RectI{0, 0, 0, 0};
    }
    return renderer2d::RectI{x0, y0, x1 - x0, y1 - y0};
}

renderer2d::RectI layer_bounds(const scene::EvaluatedLayerState& layer, std::int64_t composition_width, std::int64_t composition_height) {
    if (!layer.visible || !layer.active) {
        return renderer2d::RectI{0, 0, 0, 0};
    }

    int base_width = static_cast<int>(layer.width);
    int base_height = static_cast<int>(layer.height);

    if (base_width <= 0 || base_height <= 0) {
        if (layer.type == "text") {
            base_width = std::max(64, static_cast<int>(composition_width / 6));
            base_height = std::max(32, static_cast<int>(composition_height / 10));
        } else if (layer.type == "solid" || layer.type == "shape" || layer.type == "mask") {
            base_width = std::max(64, static_cast<int>(composition_width / 4));
            base_height = std::max(32, static_cast<int>(composition_height / 8));
        } else if (layer.precomp_id.has_value() && layer.nested_composition) {
            base_width = static_cast<int>(layer.nested_composition->width);
            base_height = static_cast<int>(layer.nested_composition->height);
        } else {
            base_width = 100;
            base_height = 100;
        }
    }

    renderer2d::RectI bounds{
        static_cast<int>(std::round(layer.position.x)),
        static_cast<int>(std::round(layer.position.y)),
        std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * layer.scale.x))),
        std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * layer.scale.y)))
    };

    return bounds;
}

} // namespace

TileGrid build_tile_grid(const scene::EvaluatedCompositionState& state, int tile_size) {
    TileGrid grid;
    grid.tile_size = std::max(1, tile_size);

    renderer2d::RectI roi{0, 0, 0, 0};
    for (const auto& layer : state.layers) {
        roi = union_rects(roi, layer_bounds(layer, state.width, state.height));
    }

    grid.roi = clamp_to_frame(roi, state.width, state.height);
    if (grid.roi.width <= 0 || grid.roi.height <= 0) {
        return grid;
    }

    for (int y = grid.roi.y; y < grid.roi.y + grid.roi.height; y += grid.tile_size) {
        for (int x = grid.roi.x; x < grid.roi.x + grid.roi.width; x += grid.tile_size) {
            const int tile_width = std::min(grid.tile_size, grid.roi.x + grid.roi.width - x);
            const int tile_height = std::min(grid.tile_size, grid.roi.y + grid.roi.height - y);
            grid.tiles.push_back(renderer2d::RectI{x, y, tile_width, tile_height});
        }
    }

    return grid;
}

} // namespace tachyon
