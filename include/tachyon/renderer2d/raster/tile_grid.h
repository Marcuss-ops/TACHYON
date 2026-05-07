#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <cstdint>
#include <vector>

namespace tachyon::renderer2d {

struct TileGrid {
    int tile_size{256};
    RectI roi{0, 0, 0, 0};
    std::vector<RectI> tiles;
};

TileGrid build_tile_grid(const RectI& roi, std::int64_t frame_width, std::int64_t frame_height, int tile_size = 256);
TileGrid build_tile_grid(const scene::EvaluatedCompositionState& state, int tile_size = 256);

} // namespace tachyon::renderer2d
