#include "tachyon/presets/background/background_preset_registry.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundPresetRegistry tests..." << std::endl;

    const auto presets = BackgroundPresetRegistry::instance().list_ids();
    assert(presets.empty());

    auto bg = BackgroundPresetRegistry::instance().create("tachyon.background.preset.galaxy_premium", 1920, 1080);
    assert(!bg.has_value());

    std::cout << "BackgroundPresetRegistry tests passed!" << std::endl;
    return true;
}
