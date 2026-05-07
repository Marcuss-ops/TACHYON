#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/registry/parameter_schema.h"
#include <string>
#include <string_view>
#include <functional>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Specification for an effect preset.
 */
struct EffectPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Factory function to create the EffectSpec based on parameters.
    std::function<EffectSpec(const registry::ParameterBag&)> factory;
};

/**
 * @brief Registry for effect presets.
 */
class EffectPresetRegistry : public registry::TypedRegistry<EffectPresetSpec> {
public:
    EffectPresetRegistry() {
        load_builtins();
    }
    ~EffectPresetRegistry() = default;

    /**
     * @brief Creates an EffectSpec instance from the specified preset.
     */
    EffectSpec create(std::string_view id, const registry::ParameterBag& params) const;

    /**
     * @brief Loads all built-in effect presets.
     */
    void load_builtins();
};

} // namespace tachyon::presets
