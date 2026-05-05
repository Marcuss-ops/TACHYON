#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundPresetRegistry tests..." << std::endl;

    const auto presets = BackgroundPresetRegistry::instance().list_ids();
    // Assuming empty for now as in original test
    assert(presets.empty());

    tachyon::registry::ParameterBag params;
    params.set("width", 1920);
    params.set("height", 1080);
    
    auto bg = BackgroundPresetRegistry::instance().create("tachyon.background.preset.galaxy_premium", params);
    assert(!bg.has_value());

    std::cout << "BackgroundPresetRegistry tests passed!" << std::endl;
    return true;
}
