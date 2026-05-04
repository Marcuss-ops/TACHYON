#include "tachyon/presets/transition/transition_preset_registry.h"
#include <cassert>
#include <iostream>

bool run_transition_preset_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running TransitionPresetRegistry tests..." << std::endl;

    auto& registry = TransitionPresetRegistry::instance();

    // 1. Test basic creation
    {
        TransitionParams p;
        p.id = "fade";
        p.duration = 0.5;
        auto spec = registry.create_enter(p);
        assert(spec.type == "fade");
        assert(spec.duration == 0.5);
    }

    // 2. Test fallback for unknown ID
    {
        TransitionParams p;
        p.id = "unknown_id_xyz";
        p.duration = 0.3;
        auto spec = registry.create_enter(p);
        assert(spec.type == "unknown_id_xyz"); // Fallback should still use the ID
        assert(spec.duration == 0.3);
    }

    // 3. Test elastic_slide custom logic
    {
        TransitionParams p;
        p.id = "elastic_slide";
        p.duration = 0.8;
        auto spec = registry.create_enter(p);
        assert(spec.type == "elastic_slide");
        assert(spec.duration == 0.8);
        assert(spec.spring.stiffness == 400.0);
    }

    // 4. Test listing IDs
    {
        auto ids = registry.list_ids();
        bool found_fade = false;
        for (const auto& id : ids) {
            if (id == "fade") found_fade = true;
        }
        assert(found_fade);
    }

    std::cout << "TransitionPresetRegistry tests passed!" << std::endl;
    return true;
}
