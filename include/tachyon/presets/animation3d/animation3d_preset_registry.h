#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Parameters for applying an animation 3D preset.
 */
struct Animation3DParams {
    double duration{1.0};
    double delay{0.0};
    float intensity{1.0f};
    bool loop{true};
};

/**
 * @brief Specification for an animation 3D preset.
 */
struct Animation3DPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Function to apply the 3D animation to a layer.
    std::function<void(LayerSpec&, const registry::ParameterBag&)> apply;
};

/**
 * @brief Registry for 3D animation presets.
 */
class Animation3DPresetRegistry {
public:
    static Animation3DPresetRegistry& instance();

    void register_spec(Animation3DPresetSpec spec);
    const Animation3DPresetSpec* find(std::string_view id) const;
    
    bool apply(std::string_view id, LayerSpec& layer, const registry::ParameterBag& params) const;

    std::vector<std::string> list_ids() const;
    void load_builtins();

private:
    Animation3DPresetRegistry();
    ~Animation3DPresetRegistry() = default;

    registry::TypedRegistry<Animation3DPresetSpec> registry_;
};

} // namespace tachyon::presets
