#include "tachyon/renderer2d/effects/effect_registry.h"
#include <utility>

namespace tachyon::renderer2d {

// Forward declarations for table descriptor functions
std::vector<EffectDescriptor> get_blur_effect_descriptors();
std::vector<EffectDescriptor> get_color_effect_descriptors();
std::vector<EffectDescriptor> get_transition_effect_descriptors(const tachyon::TransitionRegistry& transition_registry);
std::vector<EffectDescriptor> get_distortion_effect_descriptors();
std::vector<EffectDescriptor> get_generator_effect_descriptors();
std::vector<EffectDescriptor> get_stylize_effect_descriptors();

EffectRegistry::EffectRegistry() {
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

void register_builtin_effects(EffectRegistry& registry, const tachyon::TransitionRegistry& transition_registry) {
    auto register_all = [&](std::vector<EffectDescriptor> descriptors) {
        for (auto& desc : descriptors) {
            registry.register_spec(std::move(desc));
        }
    };

    register_all(get_blur_effect_descriptors());
    register_all(get_color_effect_descriptors());
    register_all(get_transition_effect_descriptors(transition_registry));
    register_all(get_distortion_effect_descriptors());
    register_all(get_generator_effect_descriptors());
    register_all(get_stylize_effect_descriptors());
}

} // namespace tachyon::renderer2d
