#include "tachyon/presets/background/background_preset_registry.h"
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

bool run_background_preset_registry_tests() {
    using namespace tachyon::presets;
    using tachyon::LayerType;

    std::cout << "Running BackgroundPresetRegistry tests..." << std::endl;

    // 1. Test listing presets
    {
        const auto presets = list_background_presets();
        auto find_id = [&](std::string_view id) {
            return std::find(presets.begin(), presets.end(), id) != presets.end();
        };

        assert(find_id("blank_canvas"));
        assert(find_id("aurora_mesh"));
        assert(find_id("liquid_glass_blobs"));
        assert(find_id("dot_grid_fade"));
        assert(find_id("noise_grain_vignette"));
        assert(find_id("mesh_gradient"));
        assert(find_id("ripple_grid"));
        assert(find_id("shape_grid_square"));
    }

    // 2. Test building each preset
    {
        const std::vector<std::string> ids = {
            "blank_canvas",
            "aurora_mesh",
            "liquid_glass_blobs",
            "dot_grid_fade",
            "noise_grain_vignette",
            "mesh_gradient",
            "ripple_grid",
            "shape_grid_square"
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

    // 3. Test registry direct access
    {
        auto& registry = BackgroundPresetRegistry::instance();
        auto bg = registry.create("aurora_mesh", 1280, 720, 10.0);
        assert(bg.has_value());
        assert(bg->width == 1280);
        assert(bg->height == 720);
        assert(bg->out_point == 10.0);
    }

    std::cout << "BackgroundPresetRegistry tests passed!" << std::endl;
    return true;
}
