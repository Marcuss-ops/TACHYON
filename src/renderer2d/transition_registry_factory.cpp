#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include <vector>

namespace tachyon {

// Forward declaration from transition_manifest.cpp
const std::vector<TransitionDescriptor>& get_transition_manifest();

void register_builtin_transitions(TransitionRegistry& reg) {
    for (const auto& desc : get_transition_manifest()) {
        reg.register_descriptor(desc);
    }
}

TransitionRegistry create_default_transition_registry() {
    TransitionRegistry registry;
    register_builtin_transitions(registry);

    // Resolve implementations (attach functions to descriptors)
    for (auto* desc : registry.list_all()) {
        if (!desc) continue;
        // Cast away const to attach implementations
        TransitionDescriptor& mutable_desc = const_cast<TransitionDescriptor&>(*desc);
        renderer2d::resolve_basic_transition_implementations(mutable_desc);
        renderer2d::resolve_artistic_transition_implementations(mutable_desc);
        renderer2d::resolve_light_leak_implementations(mutable_desc);
    }

    return registry;
}

} // namespace tachyon
