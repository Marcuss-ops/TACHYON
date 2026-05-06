#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/presets/transition/transition_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/registry/parameter_schema.h"
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a transition preset.
 */
struct TransitionPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Factory function to create the transition spec based on parameters.
    std::function<LayerTransitionSpec(const registry::ParameterBag&)> factory;
};

/**
 * @brief Registry for transition presets.
 */
class TransitionPresetRegistry : public registry::TypedRegistry<TransitionPresetSpec> {
public:
    static TransitionPresetRegistry& instance();

    /**
     * @brief Creates a transition spec instance from the specified preset.
     */
    LayerTransitionSpec create(std::string_view id, const registry::ParameterBag& params) const;

    /**
     * @brief Loads all built-in transition presets.
     */
    void load_builtins();

private:
    TransitionPresetRegistry() = default;
    ~TransitionPresetRegistry() = default;
};

} // namespace tachyon::presets
