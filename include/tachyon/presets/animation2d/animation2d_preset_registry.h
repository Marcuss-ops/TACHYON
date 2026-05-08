#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Specification for an animation 2D preset.
 */
struct Animation2DPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;

    // Function to apply the animation to a layer.
    std::function<void(LayerSpec&, const registry::ParameterBag&)> apply;
};

/**
 * @brief Registry for 2D animation presets.
 */
class Animation2DPresetRegistry : public registry::TypedRegistry<Animation2DPresetSpec> {
public:
    static Animation2DPresetRegistry& instance();

    bool apply(std::string_view id, LayerSpec& layer, const registry::ParameterBag& params) const;

    void load_builtins();

    Animation2DPresetRegistry() {
        load_builtins();
    }
    ~Animation2DPresetRegistry() = default;
};

} // namespace tachyon::presets
