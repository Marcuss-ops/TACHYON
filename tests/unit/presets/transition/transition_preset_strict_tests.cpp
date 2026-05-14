#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    using namespace tachyon;
    using namespace tachyon::presets;

    std::cout << "Running TransitionPresetRegistry strict tests..." << std::endl;

    TransitionPresetRegistry registry;
    registry::ParameterBag params;

    // Test 1: Unknown preset must throw
    bool threw = false;
    try {
        registry.create("tachyon.transition.DOES_NOT_EXIST", params);
    } catch (const std::runtime_error& e) {
        threw = true;
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }
    assert(threw);

    // Test 2: 'none' must be valid and return explicit no-op
    {
        auto spec = registry.create("none", params);
        assert(spec.kind == TransitionKind::None);
        assert(spec.type == "none");
        assert(spec.transition_id == "tachyon.transition.none");
    }

    // Test 3: empty ID must be valid and return explicit no-op
    {
        auto spec = registry.create("", params);
        assert(spec.kind == TransitionKind::None);
        assert(spec.type == "none");
        assert(spec.transition_id == "tachyon.transition.none");
    }

    // Test 4: 'tachyon.transition.none' must be valid
    {
        auto spec = registry.create("tachyon.transition.none", params);
        assert(spec.kind == TransitionKind::None);
        assert(spec.type == "none");
        assert(spec.transition_id == "tachyon.transition.none");
    }

    std::cout << "TransitionPresetRegistry strict tests passed!" << std::endl;
    return 0;
}
