#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a built-in scene preset.
 */
struct ScenePresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Function to create the scene.
    std::function<SceneSpec(const registry::ParameterBag&)> create_fn;
};

/**
 * @brief Registry for built-in scenes.
 */
class ScenePresetRegistry : public registry::TypedRegistry<ScenePresetSpec> {
public:
    static ScenePresetRegistry& instance();
    
    std::optional<SceneSpec> create(std::string_view id, const registry::ParameterBag& params) const;

    void load_builtins();

private:
    ScenePresetRegistry() = default;
    ~ScenePresetRegistry() = default;
};

} // namespace tachyon::presets
