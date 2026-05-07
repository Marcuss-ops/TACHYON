#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundPresetRegistry tests..." << std::endl;

    BackgroundPresetRegistry registry;
    const auto ids = registry.list_ids();
    assert(ids.size() >= 3); // galaxy_premium, dark_tech_grid, cinematic_aura

    tachyon::registry::ParameterBag params;
    params.set("width", 1920.0f);
    params.set("height", 1080.0f);
    params.set("duration", 2.0);

    for (const auto& id : ids) {
        auto bg = registry.create(id, params);
        assert(bg.has_value());
        assert(!bg->id.empty());
        assert(bg->preset_id == id);
        assert(bg->enabled);
        assert(bg->type != tachyon::LayerType::NullLayer);
    }

    auto bg = registry.create("tachyon.backgroundpreset.galaxy_premium", params);
    assert(bg.has_value());
    assert(!bg->id.empty());

    auto missing = registry.create("tachyon.backgroundpreset.nonexistent", params);
    assert(!missing.has_value());

    std::cout << "BackgroundPresetRegistry tests passed!" << std::endl;
    return true;
}
