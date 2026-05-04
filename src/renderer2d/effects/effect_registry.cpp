#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
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

namespace {

/**
 * @brief Helper to wrap a legacy Effect class into the new EffectDescriptor factory.
 */
template <typename T>
void register_legacy(EffectRegistry& registry, std::string id, std::string category = "general", std::vector<AuxSurfaceRequirement> aux = {}) {
    registry.register_effect({
        std::move(id),
        std::move(category),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        },
        std::move(aux)
    });
}

} // namespace

void EffectRegistry::register_builtins() {
    // ONLY KEEP HARDENED EFFECTS
    
    // Gaussian Blur: Reference implementation for standard filters
    register_legacy<GaussianBlurEffect>(*this, "gaussian_blur", "blur");

    // GLSL Transition: Reference implementation for complex effects with aux surfaces
    EffectDescriptor glsl_desc;
    glsl_desc.id = "glsl_transition";
    glsl_desc.category = "transition";
    glsl_desc.aux_requirements.push_back({"transition_to", "transition_to_layer_id", "transition", true});
    glsl_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        GlslTransitionEffect effect;
        output = effect.apply(input, params);
    };
    register_effect(std::move(glsl_desc));

    // Everything else removed to be re-added via dedicated table files.
}

} // namespace tachyon::renderer2d
