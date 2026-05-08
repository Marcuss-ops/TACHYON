#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/presets/background/background_specs.h"
#include "tachyon/presets/background/background_manifest.h"

namespace tachyon::presets {

/**
 * @brief Registry for background presets.
 * Uses BackgroundManifest as the single canonical source.
 */
class BackgroundPresetRegistry : public registry::TypedRegistry<BackgroundPresetSpec> {
public:
    /**
     * @brief Construct with BackgroundManifest to load presets from the canonical source.
     * @param manifest Reference to the BackgroundManifest that generates preset specs.
     */
    explicit BackgroundPresetRegistry(const BackgroundManifest& manifest) {
        load_from_manifest(manifest);
    }
    ~BackgroundPresetRegistry() = default;

    /**
     * @brief Creates a background layer instance from the specified preset.
     */
    std::optional<LayerSpec> create(std::string_view id, const registry::ParameterBag& params) const;

private:
    void load_from_manifest(const BackgroundManifest& manifest);
};

} // namespace tachyon::presets
