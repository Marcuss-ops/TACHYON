#pragma once

#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/presets/text/text_manifest.h"
#include <memory>

namespace tachyon::runtime {

struct RuntimeRegistryBundle {
    TransitionRegistry transitions;
    renderer2d::EffectRegistry effects;
    presets::TextManifest text_manifest;
    std::unique_ptr<presets::TextRegistry> text_registry;

    RuntimeRegistryBundle();
    ~RuntimeRegistryBundle();

    // Prevent copying because of unique_ptr and registry state
    RuntimeRegistryBundle(const RuntimeRegistryBundle&) = delete;
    RuntimeRegistryBundle& operator=(const RuntimeRegistryBundle&) = delete;

    // Allow moving
    RuntimeRegistryBundle(RuntimeRegistryBundle&&) noexcept;
    RuntimeRegistryBundle& operator=(RuntimeRegistryBundle&&) noexcept;
};

/**
 * Creates a bundle with all built-in transitions, effects, and text presets registered and resolved.
 */
RuntimeRegistryBundle create_default_runtime_registry_bundle();

} // namespace tachyon::runtime
