#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_kind_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundKindRegistry tests..." << std::endl;

    {
        const auto ids = BackgroundKindRegistry::instance().list_ids();
        auto has_id = [&](std::string_view id) {
            return std::find(ids.begin(), ids.end(), id) != ids.end();
        };

        assert(ids.size() == 17);
        assert(has_id("tachyon.background.kind.grid_modern"));
        assert(has_id("tachyon.background.kind.grid"));
        assert(has_id("tachyon.background.kind.aura"));
        assert(has_id("tachyon.background.kind.stars"));
        assert(has_id("tachyon.background.kind.noise"));
        assert(has_id("tachyon.background.kind.texture"));
        assert(has_id("tachyon.background.kind.soft_gradient"));
        assert(has_id("tachyon.background.kind.dots"));
        assert(has_id("tachyon.background.kind.silk"));
        assert(has_id("tachyon.background.kind.horizon"));
        assert(has_id("tachyon.background.kind.cyber"));
        assert(has_id("tachyon.background.kind.glass"));
        assert(has_id("tachyon.background.kind.nebula"));
        assert(has_id("tachyon.background.kind.metal"));
        assert(has_id("tachyon.background.kind.ocean"));
        assert(has_id("tachyon.background.kind.ripple_grid"));
        assert(has_id("tachyon.background.kind.classico_premium"));
    }

    {
        BackgroundParams params;
        params.kind = "tachyon.background.kind.aura";
        auto layer = build_background(params);
        assert(layer.preset_id == "tachyon.background.kind.aura");
        assert(layer.procedural.has_value());
        assert(layer.procedural->kind == "tachyon.background.kind.aura");
    }

    {
        BackgroundParams params;
        params.kind = "tachyon.background.kind.unknown_kind";
        auto layer = build_background(params);
        assert(layer.preset_id == "tachyon.background.kind.classico_premium");
        assert(layer.procedural.has_value());
    }

    std::cout << "BackgroundKindRegistry tests passed!" << std::endl;
    return true;
}
