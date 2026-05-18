#include "tachyon/runtime/execution/bounds/layer_bounds.h"
#include <iostream>

using namespace tachyon;

bool run_layer_bounds_tests() {
    std::cout << "[LayerBoundsTests] Starting...\n";
    bool ok = true;

    // Test: EmptyLayer
    {
        CompiledLayer layer;
        layer.width = 0;
        layer.height = 0;
        layer.kind = LayerType::Solid;

        auto bounds = LayerBoundsEvaluator::evaluate(layer, 0.0, 1920, 1080);
        if (!bounds.world_bounds.empty()) { std::cerr << "FAIL: bounds should be empty\n"; ok = false; }
        if (bounds.full_frame) { std::cerr << "FAIL: should not be full frame\n"; ok = false; }
    }

    // Test: ConservativeFullFrame
    {
        CompiledLayer layer;
        layer.width = 100;
        layer.height = 100;
        layer.kind = LayerType::Video;

        auto bounds = LayerBoundsEvaluator::evaluate(layer, 0.0, 1920, 1080);
        if (!bounds.full_frame) { std::cerr << "FAIL: should be full frame\n"; ok = false; }
        if (bounds.world_bounds.width != 1920 || bounds.world_bounds.height != 1080) {
            std::cerr << "FAIL: world bounds mismatch\n"; ok = false;
        }
    }

    // Test: BasicLayerLimitsToComp
    {
        CompiledLayer layer;
        layer.width = 5000;
        layer.height = 5000;
        layer.kind = LayerType::Solid;

        auto bounds = LayerBoundsEvaluator::evaluate(layer, 0.0, 1920, 1080);
        if (bounds.full_frame) { std::cerr << "FAIL: should not be full frame\n"; ok = false; }
        if (bounds.world_bounds.width != 1920 || bounds.world_bounds.height != 1080) {
            std::cerr << "FAIL: world bounds mismatch\n"; ok = false;
        }
    }

    if (ok) std::cout << "[LayerBoundsTests] ALL PASSED!\n";
    return ok;
}