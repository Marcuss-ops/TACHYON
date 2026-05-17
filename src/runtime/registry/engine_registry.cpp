#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/effect_utils.h"

namespace tachyon::runtime {

EngineRegistry::EngineRegistry() {
#ifdef TACHYON_ENABLE_TEXT
    text_registry = std::make_unique<presets::TextRegistry>(text_manifest);
#endif
}

EngineRegistry::~EngineRegistry() = default;

EngineRegistry::EngineRegistry(EngineRegistry&&) noexcept = default;
EngineRegistry& EngineRegistry::operator=(EngineRegistry&&) noexcept = default;

EngineRegistry create_default_engine_registry() {
    EngineRegistry bundle;

    // 1. Create and fully initialize transitions
    bundle.transitions = ::tachyon::create_default_transition_registry();

    // 2. Register Effects
    renderer2d::register_builtin_effects(bundle.effects, bundle.transitions);

    return bundle;
}

} // namespace tachyon::runtime
