#include "tachyon/presets/background/background_preset_registry.h"
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

bool run_background_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundPresetRegistry tests..." << std::endl;

    // 1. Test listing presets
    {
        const auto presets = list_background_presets();
        auto find_id = [&](std::string_view id) {
            return std::find(presets.begin(), presets.end(), id) != presets.end();
        };

        // NEW HARDENED PRESETS
        assert(find_id("blank_canvas"));
        assert(find_id("midnight_silk"));
        assert(find_id("cosmic_nebula"));
        assert(find_id("cyber_matrix"));
        assert(find_id("golden_horizon"));
        assert(find_id("frosted_glass"));
        assert(find_id("oceanic_abyss"));
    }

    // 2. Test building each preset
    {
        const std::vector<std::string> ids = {
            "blank_canvas",
            "midnight_silk",
            "cosmic_nebula",
            "cyber_matrix",
            "frosted_glass"
        };

        for (const auto& id : ids) {
            auto bg = build_background_preset(id, 1920, 1080);
            assert(bg.has_value());

            if (id == "blank_canvas") {
                assert(bg->type == "solid");
            } else {
                assert(bg->type == "procedural");
                assert(bg->procedural.has_value());
                assert(!bg->procedural->kind.empty());
            }
        }
    }

    std::cout << "BackgroundPresetRegistry tests passed!" << std::endl;
    return true;
}
