#pragma once

#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Manifest that consolidates all effect generation.
 * 
 * This is the single canonical source for both runtime effect descriptors
 * and authoring effect preset specs. It replaces scattered registration
 * calls across multiple files.
 */
class EffectManifest {
public:
    /**
     * @brief Construct with required dependencies.
     * @param transition_registry Reference to the transition registry for transition effects.
     *                           The manifest does NOT own this; lifetime must be managed externally.
     */
    explicit EffectManifest(const TransitionRegistry& transition_registry);

    /**
     * @brief Generate all runtime effect descriptors from all categories.
     * @return Vector of all builtin effect descriptors.
     */
    std::vector<EffectDescriptor> generate_descriptors() const;

    /**
     * @brief Generate all authoring effect preset specs from all categories.
     * @return Vector of all builtin effect preset specs.
     */
    std::vector<presets::EffectPresetSpec> generate_preset_specs() const;

private:
    const TransitionRegistry& transition_registry_;
};

} // namespace tachyon::renderer2d
