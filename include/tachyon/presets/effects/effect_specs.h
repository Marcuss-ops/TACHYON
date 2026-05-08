#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>
#include <functional>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Specification for a low-level effect kind (rendering type).
 * Defines the schema and identity of an effect that the renderer can implement.
 */
struct EffectKindSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Additional flags for the renderer
    bool is_deterministic{true};
    bool supports_gpu_acceleration{false};
};

/**
 * @brief Specification for a high-level effect preset.
 * A preset is a pre-configured effect (or combination of effects) with fixed or parameterized values.
 */
struct EffectPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    /**
     * @brief Factory function to create an EffectSpec from parameters.
     * The resulting EffectSpec points to an effect kind and provides parameters.
     */
    std::function<tachyon::EffectSpec(const registry::ParameterBag&)> factory;
};

} // namespace tachyon::presets
