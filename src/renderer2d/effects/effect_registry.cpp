#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/utility/number_counter_effect.h"
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
void register_legacy(EffectRegistry& registry, std::string id, std::string category = "general") {
    registry.register_effect({
        std::move(id),
        std::move(category),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        }
    });
}

} // namespace

void EffectRegistry::register_builtins() {
    register_legacy<GaussianBlurEffect>(*this, "gaussian_blur", "blur");
    register_legacy<DirectionalBlurEffect>(*this, "directional_blur", "blur");
    register_legacy<RadialBlurEffect>(*this, "radial_blur", "blur");
    register_legacy<DropShadowEffect>(*this, "drop_shadow", "stylize");
    register_legacy<GlowEffect>(*this, "glow", "stylize");
    register_legacy<LevelsEffect>(*this, "levels", "color");
    register_legacy<CurvesEffect>(*this, "curves", "color");
    register_legacy<FillEffect>(*this, "fill", "generate");
    register_legacy<TintEffect>(*this, "tint", "color");
    register_legacy<HueSaturationEffect>(*this, "hue_saturation", "color");
    register_legacy<ColorBalanceEffect>(*this, "color_balance", "color");
    register_legacy<LUTEffect>(*this, "lut", "color");
    register_legacy<Lut3DCubeEffect>(*this, "lut3d_cube", "color");
    register_legacy<ChromaticAberrationEffect>(*this, "chromatic_aberration", "distort");
    register_legacy<VignetteEffect>(*this, "vignette", "stylize");
    register_legacy<ParticleEmitterEffect>(*this, "particle_emitter", "generate");
    register_legacy<DisplacementMapEffect>(*this, "displacement_map", "distort");
    register_legacy<ChromaKeyEffect>(*this, "chroma_key", "keying");
    register_legacy<LightWrapEffect>(*this, "light_wrap", "keying");
    register_legacy<MatteRefinementEffect>(*this, "matte_refinement", "keying");
    register_legacy<VectorBlurEffect>(*this, "vector_blur", "blur");
    register_legacy<MotionBlur2DEffect>(*this, "motion_blur_2d", "blur");
    register_legacy<NumberCounterEffect>(*this, "number_counter", "generate");

    // Special case for GLSL Transition - it needs an auxiliary surface
    EffectDescriptor glsl_desc;
    glsl_desc.id = "glsl_transition";
    glsl_desc.category = "transition";
    glsl_desc.aux_requirements.push_back({"transition_to", "The target surface to transition into."});
    glsl_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        GlslTransitionEffect effect;
        output = effect.apply(input, params);
    };
    register_effect(std::move(glsl_desc));
}

} // namespace tachyon::renderer2d
