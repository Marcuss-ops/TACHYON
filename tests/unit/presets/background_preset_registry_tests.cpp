#include "tachyon/presets/background/background_preset_registry.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundPresetRegistry tests..." << std::endl;

    const auto presets = BackgroundPresetRegistry::instance().list_ids();
    auto has_id = [&](std::string_view id) {
        return std::find(presets.begin(), presets.end(), id) != presets.end();
    };

    assert(presets.size() == 1);
    assert(has_id("tachyon.background.preset.galaxy_premium"));

    auto bg = BackgroundPresetRegistry::instance().create("tachyon.background.preset.galaxy_premium", 1920, 1080);
    assert(bg.has_value());
    assert(bg->type == "procedural");
    assert(bg->preset_id == "tachyon.background.preset.galaxy_premium");
    assert(bg->procedural.has_value());

    std::cout << "BackgroundPresetRegistry tests passed!" << std::endl;
    return true;
}
