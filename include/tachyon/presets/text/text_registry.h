#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
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
 * @brief Specification for a text layer style preset.
 */
struct TextLayerPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    std::function<LayerSpec(const registry::ParameterBag&)> factory;
};

/**
 * @brief Specification for a text animator preset.
 */
struct TextAnimatorPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    std::function<std::vector<TextAnimatorSpec>(const registry::ParameterBag&)> factory;
};

/**
 * @brief Unified registry for all text-related presets.
 */
class TextRegistry {
public:
    explicit TextRegistry(const TextManifest& manifest);
    std::optional<LayerSpec> create_layer(std::string_view id, const registry::ParameterBag& params) const;
    std::vector<TextAnimatorSpec> create_animators(std::string_view id, const registry::ParameterBag& params) const;

    // Access to underlying registries
    const registry::TypedRegistry<TextLayerPresetSpec>& layers() const { return layers_; }
    const registry::TypedRegistry<TextAnimatorPresetSpec>& animators() const { return animators_; }

private:
    void load_from_manifest(const TextManifest& manifest);

    registry::TypedRegistry<TextLayerPresetSpec> layers_;
    registry::TypedRegistry<TextAnimatorPresetSpec> animators_;
};

} // namespace tachyon::presets
