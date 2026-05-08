#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/text/text_manifest.h"
#include "tachyon/presets/text/text_params.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a text layer style preset.
 */
struct TextLayerPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Factory function to create a text layer.
    std::function<LayerSpec(const registry::ParameterBag&)> factory;
};

/**
 * @brief Registry for text layer style presets.
 * Uses TextManifest as the single canonical source.
 */
class TextLayerPresetRegistry : public registry::TypedRegistry<TextLayerPresetSpec> {
public:
    /**
     * @brief Construct with TextManifest to load presets from the canonical source.
     * @param manifest Reference to the TextManifest that generates preset specs.
     */
    explicit TextLayerPresetRegistry(const TextManifest& manifest) {
        load_from_manifest(manifest);
    }
    ~TextLayerPresetRegistry() = default;

    /**
     * @brief Creates a text layer instance from the specified preset.
     */
    std::optional<LayerSpec> create(std::string_view id, const registry::ParameterBag& params) const;

private:
    void load_from_manifest(const TextManifest& manifest);
};

} // namespace tachyon::presets
