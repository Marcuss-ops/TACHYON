#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/effect_utils.h"

namespace tachyon::runtime {

RuntimeRegistryBundle::RuntimeRegistryBundle() 
    : text_registry(std::make_unique<presets::TextRegistry>(text_manifest)) {}

RuntimeRegistryBundle::~RuntimeRegistryBundle() = default;

RuntimeRegistryBundle::RuntimeRegistryBundle(RuntimeRegistryBundle&&) noexcept = default;
RuntimeRegistryBundle& RuntimeRegistryBundle::operator=(RuntimeRegistryBundle&&) noexcept = default;

RuntimeRegistryBundle create_default_runtime_registry_bundle() {
    RuntimeRegistryBundle bundle;

    // 1. Create and fully initialize transitions
    bundle.transitions = ::tachyon::create_default_transition_registry();

    // 2. Register Effects
    renderer2d::register_builtin_effects(bundle.effects, bundle.transitions);

    return bundle;
}

} // namespace tachyon::runtime
