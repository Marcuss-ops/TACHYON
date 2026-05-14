#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    using namespace tachyon;

    std::cout << "Running TransitionRegistry duplicate policy tests..." << std::endl;

    TransitionDescriptor a;
    a.id = "tachyon.transition.test";
    a.display_name = "A";

    TransitionDescriptor b;
    b.id = "tachyon.transition.test";
    b.display_name = "B";

    // --- Reject Policy ---
    {
        TransitionRegistry registry;
        registry.set_duplicate_policy(RegistryDuplicatePolicy::Reject);
        registry.register_descriptor(a);

        bool threw = false;
        try {
            registry.register_descriptor(b);
        } catch (const RegistryError& e) {
            threw = true;
            std::cout << "Caught expected RegistryError: " << e.what() << std::endl;
        }
        assert(threw);
    }

    // --- Warn Policy ---
    {
        TransitionRegistry registry;
        registry.set_duplicate_policy(RegistryDuplicatePolicy::Warn);
        registry.register_descriptor(a);
        registry.register_descriptor(b); // Should warn and ignore

        auto* found = registry.find_by_id("tachyon.transition.test");
        assert(found != nullptr);
        assert(found->display_name == "A");
    }

    // --- Replace Policy ---
    {
        TransitionRegistry registry;
        registry.set_duplicate_policy(RegistryDuplicatePolicy::Replace);
        registry.register_descriptor(a);
        registry.register_descriptor(b); // Should replace

        auto* found = registry.find_by_id("tachyon.transition.test");
        assert(found != nullptr);
        assert(found->display_name == "B");
    }

    std::cout << "TransitionRegistry duplicate policy tests passed!" << std::endl;
    return 0;
}
