#include "tachyon/runtime/tile_scheduler.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_tile_scheduler_tests() {
    using namespace tachyon;

    g_failures = 0;

    scene::EvaluatedCompositionState state;
    state.width = 1024;
    state.height = 768;

    scene::EvaluatedLayerState layer;
    layer.visible = true;
    layer.active = true;
    layer.type = scene::LayerType::Shape;
    layer.local_transform.position = {32.0f, 48.0f};
    layer.local_transform.scale = {1.0f, 1.0f};
    layer.width = 128;
    layer.height = 96;
    state.layers.push_back(layer);

    const TileGrid grid = build_tile_grid(state, 256);
    check_true(grid.roi.width > 0 && grid.roi.height > 0, "ROI should be non-empty for visible content");
    check_true(grid.tiles.size() >= 1, "Tile grid should produce at least one tile");
    check_true(grid.tiles[0].x == 32 && grid.tiles[0].y == 48, "First tile should start at ROI origin");
    check_true(grid.tiles[0].width <= 256 && grid.tiles[0].height <= 256, "Tile size should be capped by tile size");

    scene::EvaluatedCompositionState empty_state;
    empty_state.width = 640;
    empty_state.height = 360;
    const TileGrid empty_grid = build_tile_grid(empty_state, 256);
    check_true(empty_grid.tiles.empty(), "Empty composition should have no tiles");

    return g_failures == 0;
}
