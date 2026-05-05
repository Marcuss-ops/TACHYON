#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <cassert>
#include <iostream>

bool run_transition_preset_registry_tests() {
    using namespace tachyon::presets;
    using tachyon::TransitionKind;

    std::cout << "Running TransitionPresetRegistry tests..." << std::endl;

    auto& preset_registry = TransitionPresetRegistry::instance();

    {
        auto ids = preset_registry.list_ids();
        // 19 GLSL presets + 'none'
        assert(ids.size() == 20);
        assert(preset_registry.find("tachyon.transition.crossfade") != nullptr);
        assert(preset_registry.find("tachyon.transition.slide_up") != nullptr);
        assert(preset_registry.find("tachyon.transition.swipe_left") != nullptr);
    }

    // 2. Known ids still resolve to the registered preset.
    {
        tachyon::registry::ParameterBag params;
        params.set("id", std::string("tachyon.transition.crossfade"));
        params.set("duration", 0.5);

        auto spec = preset_registry.create("tachyon.transition.crossfade", params);
        assert(spec.type == "tachyon.transition.crossfade");
        assert(spec.transition_id == "tachyon.transition.crossfade");
        assert(spec.duration == 0.5);
    }

    // 3. Unknown ids are preserved as explicit no-op specs.
    {
        tachyon::registry::ParameterBag params;
        params.set("id", std::string("tachyon.transition.missing_preset"));
        params.set("duration", 0.8);
        params.set("delay", 0.25);

        auto spec = preset_registry.create("tachyon.transition.missing_preset", params);
        assert(spec.type == "none");
        assert(spec.kind == TransitionKind::None);
        assert(spec.transition_id == "tachyon.transition.missing_preset");
        assert(spec.duration == 0.8);
        assert(spec.delay == 0.25);
    }

    std::cout << "TransitionPresetRegistry tests passed!" << std::endl;
    return true;
}
