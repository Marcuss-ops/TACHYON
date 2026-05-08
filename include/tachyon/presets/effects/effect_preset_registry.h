#pragma once

#include "tachyon/presets/effects/effect_specs.h"
#include "tachyon/core/registry/typed_registry.h"
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::presets {

class EffectManifest;

/**
 * @brief Registry for effect presets, using EffectManifest as the canonical source.
 */
class EffectPresetRegistry : public registry::TypedRegistry<EffectPresetSpec> {
public:
    /**
     * @brief Construct an empty registry.
     */
    EffectPresetRegistry() = default;

    /**
     * @brief Construct with EffectManifest to load presets from the canonical source.
     * @param manifest Reference to the EffectManifest that generates preset specs.
     */
    explicit EffectPresetRegistry(const EffectManifest& manifest);
    ~EffectPresetRegistry() = default;

    /**
     * @brief Load presets from the specified manifest.
     */
    void load_from_manifest(const EffectManifest& manifest);

    /**
     * @brief Creates an EffectSpec instance from the specified preset.
     */
    ::tachyon::EffectSpec create(std::string_view id, const registry::ParameterBag& params) const;
};

} // namespace tachyon::presets
