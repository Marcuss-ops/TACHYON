#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/presets/transition/transition_params.h"
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
class TransitionPresetRegistry {
public:
    static TransitionPresetRegistry& instance();

    void register_spec(TransitionPresetSpec spec);
    const TransitionPresetSpec* find(std::string_view id) const;

    LayerTransitionSpec create(std::string_view id, const registry::ParameterBag& params) const;

    std::vector<std::string> list_ids() const;

    void load_builtins();

private:
    TransitionPresetRegistry();
    ~TransitionPresetRegistry() = default;

    registry::TypedRegistry<TransitionPresetSpec> registry_;
};

} // namespace tachyon::presets
