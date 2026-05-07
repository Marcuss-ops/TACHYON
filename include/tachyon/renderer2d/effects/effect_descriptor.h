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
    std::string_view id;
    std::string_view display_name;
    std::string_view category; // Metadata category becomes "effect." + category
    std::string_view description; // Optional
    std::vector<std::string_view> tags; // Optional
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
 * @brief Forward declaration of EffectRegistry to avoid circular include.
 */
class EffectRegistry;

/**
 * @brief Register a builtin effect from a declarative spec.
 * Implementation moved to avoid circular dependencies.
 */
TACHYON_API void register_effect_from_spec(EffectRegistry& registry, const EffectBuiltinSpec& spec);

} // namespace tachyon::renderer2d
