#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/effect_utils.h"
#include "tachyon/presets/effects/effect_manifest.h"

namespace tachyon::runtime {

RuntimeRegistryBundle::RuntimeRegistryBundle() 
    : text_registry(std::make_unique<presets::TextRegistry>(text_manifest)) {}

RuntimeRegistryBundle::~RuntimeRegistryBundle() = default;

RuntimeRegistryBundle::RuntimeRegistryBundle(RuntimeRegistryBundle&&) noexcept = default;
RuntimeRegistryBundle& RuntimeRegistryBundle::operator=(RuntimeRegistryBundle&&) noexcept = default;

RuntimeRegistryBundle create_default_runtime_registry_bundle() {
    RuntimeRegistryBundle bundle;

    // 1. Register Transitions
    ::tachyon::register_builtin_transitions(bundle.transitions);

    // 2. Resolve Transition Implementations
    for (auto* desc : bundle.transitions.list_all()) {
        if (!desc) continue;
        renderer2d::resolve_basic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_artistic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_light_leak_implementations(const_cast<TransitionDescriptor&>(*desc));
    }

    // 3. Register Effects
    presets::EffectManifest effect_manifest;
    renderer2d::register_builtin_effects(bundle.effects, effect_manifest, bundle.transitions);

    return bundle;
}

} // namespace tachyon::runtime
