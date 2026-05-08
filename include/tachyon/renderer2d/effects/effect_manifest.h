#pragma once

#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/effects/effect_manifest.h"
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Bridge between presets::EffectManifest (authoring) and runtime implementations.
 */
class EffectManifest {
public:
    explicit EffectManifest(const TransitionRegistry& transition_registry);

    /**
     * @brief Generate all runtime effect descriptors.
     * Joins kind specs from presets with apply functions from renderer2d.
     */
    std::vector<EffectDescriptor> generate_descriptors() const;

private:
    const TransitionRegistry& transition_registry_;
    presets::EffectManifest presets_manifest_;
};

} // namespace tachyon::renderer2d
