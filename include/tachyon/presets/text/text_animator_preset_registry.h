#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/presets/text/text_manifest.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a text animator preset.
 */
struct TextAnimatorPresetSpec {
    using FactoryFn = std::function<std::vector<TextAnimatorSpec>(const registry::ParameterBag&)>;
    
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    FactoryFn factory;
};

/**
 * @brief Registry for text animator presets.
 * Uses TextManifest as the single canonical source.
 */
class TextAnimatorPresetRegistry : public registry::TypedRegistry<TextAnimatorPresetSpec> {
public:
    /**
     * @brief Construct with TextManifest to load presets from the canonical source.
     * @param manifest Reference to the TextManifest that generates preset specs.
     */
    explicit TextAnimatorPresetRegistry(const TextManifest& manifest) {
        load_from_manifest(manifest);
    }
    ~TextAnimatorPresetRegistry() = default;

    /**
     * @brief Creates text animator specs from the specified preset.
     */
    std::vector<TextAnimatorSpec> create(std::string_view id, const registry::ParameterBag& params) const;

private:
    void load_from_manifest(const TextManifest& manifest);
};

} // namespace tachyon::presets
