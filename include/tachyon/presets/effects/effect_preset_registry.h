#pragma once

#include "tachyon/presets/effects/effect_specs.h"
#include "tachyon/core/registry/typed_registry.h"
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Registry for effect presets.
 */
class EffectPresetRegistry : public registry::TypedRegistry<EffectPresetSpec> {
public:
    /**
     * @brief Access the global singleton instance.
     */
    TACHYON_API static const EffectPresetRegistry& instance();

    /**
     * @brief Construct a registry preloaded with builtin effect presets.
     */
    EffectPresetRegistry();

    ~EffectPresetRegistry() = default;

    /**
     * @brief Creates an EffectSpec instance from the specified preset.
     */
    ::tachyon::EffectSpec create(std::string_view id, const registry::ParameterBag& params) const;
};

} // namespace tachyon::presets
