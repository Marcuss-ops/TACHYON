#include "tachyon/renderer2d/effects/effect_registry.h"
#include <utility>

namespace tachyon::renderer2d {

// Forward declarations for table registration functions
void register_blur_effects(EffectRegistry& registry);
void register_color_effects(EffectRegistry& registry);
void register_transition_effects(EffectRegistry& registry);
void register_distortion_effects(EffectRegistry& registry);
void register_glitch_effects(EffectRegistry& registry);

EffectRegistry& EffectRegistry::instance() {
    static EffectRegistry instance;
    return instance;
}

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
    register_glitch_effects(*this);
}

} // namespace tachyon::renderer2d
