#include "tachyon/renderer2d/effects/effect_registry.h"
#include <iostream>

namespace tachyon::renderer2d {

EffectRegistry& EffectRegistry::instance() {
    static EffectRegistry instance;
    return instance;
}

void EffectRegistry::register_effect(EffectDescriptor descriptor) {
    if (descriptor.id.empty()) return;
    m_effects[descriptor.id] = std::move(descriptor);
}

const EffectDescriptor* EffectRegistry::find(const std::string& id) const {
    auto it = m_effects.find(id);
    if (it != m_effects.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const EffectDescriptor*> EffectRegistry::get_all() const {
    std::vector<const EffectDescriptor*> result;
    result.reserve(m_effects.size());
    for (const auto& [id, desc] : m_effects) {
        result.push_back(&desc);
    }
    return result;
}

void EffectRegistry::register_builtins() {
    // This will be populated as we migrate individual effects to the new system.
    // For now, it's a placeholder to show the architecture.
    
    // Example migration for 'fill':
    /*
    register_effect({
        "fill",
        "color",
        [](const EffectSpec& spec, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&) {
            // New factory implementation calling the actual logic
        }
    });
    */
}

} // namespace tachyon::renderer2d
