#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/scene/evaluated_state.h"

#include <cstdint>
#include <vector>

namespace tachyon {

struct TileGrid {
    int tile_size{256};
    renderer2d::RectI roi{0, 0, 0, 0};
    std::vector<renderer2d::RectI> tiles;
};

TileGrid build_tile_grid(const renderer2d::RectI& roi, std::int64_t frame_width, std::int64_t frame_height, int tile_size = 256);
TileGrid build_tile_grid(const scene::EvaluatedCompositionState& state, int tile_size = 256);

} // namespace tachyon
