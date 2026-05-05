#include "tachyon/presets/transition/transition_preset_registry.h"
#include <cassert>
#include <iostream>

bool run_transition_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running TransitionPresetRegistry tests..." << std::endl;

    auto& registry = TransitionPresetRegistry::instance();

    {
        auto ids = registry.list_ids();
        // 19 GLSL presets + 'none'
        assert(ids.size() == 20);
        assert(registry.find("tachyon.transition.crossfade") != nullptr);
        assert(registry.find("tachyon.transition.slide_up") != nullptr);
        assert(registry.find("tachyon.transition.swipe_left") != nullptr);
    }

    // 2. Unknown ids still round-trip through the generic fallback.
    {
        TransitionParams p;
        p.id = "tachyon.transition.crossfade";
        p.duration = 0.5;
        auto spec = registry.create(p.id, p);
        assert(spec.type == "tachyon.transition.crossfade");
        assert(spec.transition_id == "tachyon.transition.crossfade");
        assert(spec.duration == 0.5);
    }

    // 3. Another unknown id behaves the same way.
    {
        TransitionParams p;
        p.id = "tachyon.transition.circle_iris";
        p.duration = 0.8;
        auto spec = registry.create(p.id, p);
        assert(spec.type == "tachyon.transition.circle_iris");
        assert(spec.transition_id == "tachyon.transition.circle_iris");
    }

    std::cout << "TransitionPresetRegistry tests passed!" << std::endl;
    return true;
}
