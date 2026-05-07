#include "tachyon/runtime/execution/scheduling/tile_scheduler.h"
#include "tachyon/renderer2d/raster/tile_grid.h"

namespace tachyon {

TileGrid build_tile_grid(const renderer2d::RectI& roi, std::int64_t frame_width, std::int64_t frame_height, int tile_size) {
    auto grid2d = renderer2d::build_tile_grid(roi, frame_width, frame_height, tile_size);
    TileGrid grid;
    grid.tile_size = grid2d.tile_size;
    grid.roi = grid2d.roi;
    grid.tiles = std::move(grid2d.tiles);
    return grid;
}

TileGrid build_tile_grid(const scene::EvaluatedCompositionState& state, int tile_size) {
    auto grid2d = renderer2d::build_tile_grid(state, tile_size);
    TileGrid grid;
    grid.tile_size = grid2d.tile_size;
    grid.roi = grid2d.roi;
    grid.tiles = std::move(grid2d.tiles);
    return grid;
}

} // namespace tachyon
