#include "tachyon/renderer2d/effects/effect_registry.h"
#include <utility>

namespace tachyon::renderer2d {

// Forward declarations for table registration functions
void register_blur_effects(EffectRegistry& registry);
void register_color_effects(EffectRegistry& registry);
void register_transition_effects(EffectRegistry& registry);
void register_distortion_effects(EffectRegistry& registry);
void register_generator_effects(EffectRegistry& registry);
void register_stylize_effects(EffectRegistry& registry);

EffectRegistry::EffectRegistry() {
    register_builtins();
}

void EffectRegistry::register_spec(EffectDescriptor descriptor) {
    if (descriptor.id.empty()) {
        return;
    }
    registry_.register_spec(std::move(descriptor));
}

const EffectDescriptor* EffectRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::vector<std::string> EffectRegistry::list_ids() const {
    return registry_.list_ids();
}

void EffectRegistry::register_builtins() {
    register_blur_effects(*this);
    register_color_effects(*this);
    register_transition_effects(*this);
    register_distortion_effects(*this);
    register_generator_effects(*this);
    register_stylize_effects(*this);
}

void register_effect_from_spec(EffectRegistry& registry, const EffectBuiltinSpec& spec) {
    std::string description = std::string(spec.description);
    if (description.empty()) {
        description = "Professional " + std::string(spec.category) + " effect.";
    }
    
    std::vector<std::string> tags;
    if (spec.tags.empty()) {
        tags = {std::string(spec.category), std::string(spec.category)};
    } else {
        tags.reserve(spec.tags.size());
        for (auto t : spec.tags) {
            tags.push_back(std::string(t));
        }
    }
    
    registry.register_spec({
        std::string(spec.id),
        {std::string(spec.id), std::string(spec.display_name), std::move(description), "effect." + std::string(spec.category), std::move(tags)},
        spec.schema,
        spec.factory,
        spec.aux_requirements,
        spec.is_deterministic,
        spec.supports_gpu_acceleration
    });
}

} // namespace tachyon::renderer2d
