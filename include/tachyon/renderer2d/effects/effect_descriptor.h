#pragma once

#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {
    struct EffectParams;
}
#include <string>
#include <functional>
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Requirements for auxiliary surfaces.
 */
struct AuxSurfaceRequirement {
    std::string param_name;
    std::string source_key;
    std::string semantic;
    bool is_required{true};
    std::string default_id;
};

/**
 * @brief Descriptor for a rendering effect.
 */
struct EffectDescriptor {
    using Factory = std::function<void(
        const EffectSpec& spec,
        const SurfaceRGBA& input,
        SurfaceRGBA& output,
        const std::vector<const SurfaceRGBA*>& aux_surfaces,
        const EffectParams& params)>;

    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    Factory factory;
    
    std::vector<AuxSurfaceRequirement> aux_requirements;
    
    bool is_deterministic{true};
    bool supports_gpu_acceleration{false};
};

/**
 * @brief Mini-spec for declarative builtin effect registration.
 */
struct EffectBuiltinSpec {
    std::string id;
    std::string display_name;
    std::string category; // Metadata category becomes "effect." + category
    std::string description; // Falls back to "Professional " + category + " effect."
    std::vector<std::string> tags; // Falls back to {category, category}
    registry::ParameterSchema schema;
    EffectDescriptor::Factory factory; // Required: use make_effect_factory<T>() for common case
    std::vector<AuxSurfaceRequirement> aux_requirements;
    bool is_deterministic{true};
    bool supports_gpu_acceleration{false};
};

/**
 * @brief Create default factory for effect type T with apply(input, params) method.
 */
template <typename T>
EffectDescriptor::Factory make_effect_factory() {
    return [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output,
              const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        T effect;
        output = effect.apply(input, params);
    };
}

/**
 * @brief Register a builtin effect from a declarative spec.
 */
inline void register_effect_from_spec(EffectRegistry& registry, EffectBuiltinSpec spec) {
    if (spec.description.empty()) {
        spec.description = "Professional " + spec.category + " effect.";
    }
    if (spec.tags.empty()) {
        spec.tags = {spec.category, spec.category};
    }
    registry.register_spec({
        spec.id,
        {spec.id, spec.display_name, spec.description, "effect." + spec.category, std::move(spec.tags)},
        std::move(spec.schema),
        std::move(spec.factory),
        std::move(spec.aux_requirements),
        spec.is_deterministic,
        spec.supports_gpu_acceleration
    });
}

} // namespace tachyon::renderer2d
