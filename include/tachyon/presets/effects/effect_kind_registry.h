#pragma once

#include "tachyon/presets/effects/effect_specs.h"
#include "tachyon/core/registry/typed_registry.h"
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::presets {

class EffectManifest;

/**
 * @brief Registry for effect kinds (low-level rendering types).
 * Consolidates all rendering effects available in the engine.
 */
class EffectKindRegistry : public registry::TypedRegistry<EffectKindSpec> {
public:
    EffectKindRegistry() = default;
    
    /**
     * @brief Construct with EffectManifest to load kinds from the canonical source.
     */
    explicit EffectKindRegistry(const EffectManifest& manifest);
    ~EffectKindRegistry() = default;

    /**
     * @brief Load kinds from the specified manifest.
     */
    void load_from_manifest(const EffectManifest& manifest);
};

} // namespace tachyon::presets
