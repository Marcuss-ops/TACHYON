#pragma once

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
 * @brief Parameters for applying an animation preset.
 */
struct Animation2DParams {
    double duration{1.0};
    double delay{0.0};
    float intensity{1.0f};
    bool loop{false};
};

/**
 * @brief Specification for an animation 2D preset.
 */
struct Animation2DPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    
    // Function to apply the animation to a layer.
    std::function<void(LayerSpec&, const Animation2DParams&)> apply;
};

/**
 * @brief Registry for 2D animation presets.
 */
class Animation2DPresetRegistry {
public:
    static Animation2DPresetRegistry& instance();

    void register_spec(Animation2DPresetSpec spec);
    const Animation2DPresetSpec* find(std::string_view id) const;
    
    bool apply(std::string_view id, LayerSpec& layer, const Animation2DParams& params) const;

    std::vector<std::string> list_ids() const;
    void load_builtins();

private:
    Animation2DPresetRegistry();
    ~Animation2DPresetRegistry() = default;

    registry::TypedRegistry<Animation2DPresetSpec> registry_;
};

} // namespace tachyon::presets
