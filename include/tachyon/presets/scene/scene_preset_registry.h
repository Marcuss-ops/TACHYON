#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Specification for a built-in scene preset.
 */
struct ScenePresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Function to create the scene.
    std::function<SceneSpec(const registry::ParameterBag&)> create;
};

/**
 * @brief Registry for built-in scenes.
 */
class ScenePresetRegistry {
public:
    static ScenePresetRegistry& instance();

    void register_spec(ScenePresetSpec spec);
    const ScenePresetSpec* find(std::string_view id) const;
    
    std::optional<SceneSpec> create(std::string_view id, const registry::ParameterBag& params) const;

    std::vector<std::string> list_ids() const;
    void load_builtins();

private:
    ScenePresetRegistry();
    ~ScenePresetRegistry() = default;

    registry::TypedRegistry<ScenePresetSpec> registry_;
};

} // namespace tachyon::presets
