#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/registry/parameter_schema.h"
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <optional>

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
 * @brief Registry for background presets.
 * Consolidated using TypedRegistry to reduce boilerplate.
 */
class BackgroundPresetRegistry : public registry::TypedRegistry<BackgroundPresetSpec> {
public:
    BackgroundPresetRegistry() {
        load_builtins();
    }
    ~BackgroundPresetRegistry() = default;

    /**
     * @brief Creates a background layer instance from the specified preset.
     */
    std::optional<LayerSpec> create(std::string_view id, const registry::ParameterBag& params) const;

    /**
     * @brief Loads all built-in background presets.
     */
    void load_builtins();
};

} // namespace tachyon::presets
