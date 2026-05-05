#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/registry/parameter_schema.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
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
 */
class BackgroundPresetRegistry {
public:
    static BackgroundPresetRegistry& instance();

    void register_spec(BackgroundPresetSpec spec);
    const BackgroundPresetSpec* find(std::string_view id) const;

    std::optional<LayerSpec> create(std::string_view id, const registry::ParameterBag& params) const;

    std::vector<std::string> list_ids() const;

    void load_builtins();

private:
    BackgroundPresetRegistry();
    ~BackgroundPresetRegistry() = default;

    registry::TypedRegistry<BackgroundPresetSpec> registry_;
};

} // namespace tachyon::presets
