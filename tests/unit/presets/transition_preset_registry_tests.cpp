#include "tachyon/presets/transition/transition_preset_registry.h"
#include <cassert>
#include <iostream>

bool run_transition_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running TransitionPresetRegistry tests..." << std::endl;

    auto& registry = TransitionPresetRegistry::instance();

    // 1. Test crossfade (GLSL based)
    {
        TransitionParams p;
        p.id = "crossfade";
        p.duration = 0.5;
        auto spec = registry.create_enter(p);
        assert(spec.type == "glsl_transition");
        assert(spec.transition_id == "fade");
        assert(spec.duration == 0.5);
    }

    // 2. Test circle_iris
    {
        TransitionParams p;
        p.id = "circle_iris";
        p.duration = 0.8;
        auto spec = registry.create_enter(p);
        assert(spec.type == "glsl_transition");
        assert(spec.transition_id == "circle_iris");
    }

    // 3. Test listing IDs
    {
        auto ids = registry.list_ids();
        assert(ids.size() >= 10);
        bool found = false;
        for (const auto& id : ids) {
            if (id == "zoom_blur") found = true;
        }
        assert(found);
    }

    std::cout << "TransitionPresetRegistry tests passed!" << std::endl;
    return true;
}
