#pragma once

#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#ifdef TACHYON_ENABLE_TEXT
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/presets/text/text_manifest.h"
#endif
#include <memory>

namespace tachyon::runtime {

struct EngineRegistry {
    TransitionRegistry transitions;
    renderer2d::EffectRegistry effects;
#ifdef TACHYON_ENABLE_TEXT
    presets::TextManifest text_manifest;
    std::unique_ptr<presets::TextRegistry> text_registry;
#endif

    EngineRegistry();
    ~EngineRegistry();

    // Prevent copying because of unique_ptr and registry state
    EngineRegistry(const EngineRegistry&) = delete;
    EngineRegistry& operator=(const EngineRegistry&) = delete;

    // Allow moving
    EngineRegistry(EngineRegistry&&) noexcept;
    EngineRegistry& operator=(EngineRegistry&&) noexcept;
};

/**
 * Creates a bundle with all built-in transitions, effects, and text presets registered and resolved.
 */
EngineRegistry create_default_engine_registry();

/**
 * @brief Legacy alias for the engine registry.
 */
using EngineRegistry = EngineRegistry;

/**
 * @brief Legacy factory alias.
 */
inline EngineRegistry create_default_runtime_registry_bundle() {
    return create_default_engine_registry();
}

} // namespace tachyon::runtime

namespace tachyon {
    using EngineRegistry = runtime::EngineRegistry;
    using EngineRegistry = runtime::EngineRegistry;
}
