#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <string>
#include <functional>

namespace tachyon::presets {

/**
 * @brief Specification for a background preset.
 */
struct BackgroundPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Factory function to create the background layer.
    std::function<LayerSpec(const registry::ParameterBag&)> factory;
};

/**
 * @brief Specification for a background kind (procedural type).
 */
struct BackgroundKindSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Factory function to create the procedural spec.
    std::function<ProceduralSpec(const registry::ParameterBag&)> factory;
};

} // namespace tachyon::presets
