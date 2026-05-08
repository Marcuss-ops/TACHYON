#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <string>
#include <string_view>
#include <functional>
#include <vector>

namespace tachyon::renderer2d { class EffectManifest; }

namespace tachyon::presets {

/**
 * @brief Specification for an effect preset.
 */
struct EffectPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    
    // Factory function to create the EffectSpec based on parameters.
    std::function<tachyon::EffectSpec(const registry::ParameterBag&)> factory;
};

/**
 * @brief Registry for effect presets, using EffectManifest as the canonical source.
 */
class EffectPresetRegistry : public registry::TypedRegistry<EffectPresetSpec> {
public:
    /**
     * @brief Construct with EffectManifest to load presets from the canonical source.
     * @param manifest Reference to the EffectManifest that generates preset specs.
     */
    explicit EffectPresetRegistry(const renderer2d::EffectManifest& manifest);
    ~EffectPresetRegistry() = default;

    /**
     * @brief Creates an EffectSpec instance from the specified preset.
     */
    tachyon::EffectSpec create(std::string_view id, const registry::ParameterBag& params) const;

private:
    void load_from_manifest(const renderer2d::EffectManifest& manifest);
};

} // namespace tachyon::presets
