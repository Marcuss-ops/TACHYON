#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include <stdexcept>
#include <utility>

namespace tachyon::renderer2d {

// Concrete implementation of EffectHost
class EffectHostImpl : public EffectHost {
public:
    void register_effect(std::string name, std::unique_ptr<Effect> effect) override {
        if (!effect) throw std::invalid_argument("effect must not be null");
        m_effects[std::move(name)] = std::move(effect);
    }
    
    bool has_effect(const std::string& name) const override {
        return m_effects.find(name) != m_effects.end();
    }
    
    SurfaceRGBA apply(const std::string& name, const SurfaceRGBA& input, const EffectParams& params) const override {
        auto it = m_effects.find(name);
        if (it == m_effects.end()) return input;
        return it->second->apply(input, params);
    }
    
    SurfaceRGBA apply_pipeline(const SurfaceRGBA& input, const std::vector<std::pair<std::string, EffectParams>>& pipeline) const override {
        SurfaceRGBA current = input;
        for (const auto& step : pipeline) {
            auto it = m_effects.find(step.first);
            if (it != m_effects.end()) {
                current = it->second->apply(current, step.second);
            }
        }
        return current;
    }
};

std::unique_ptr<EffectHost> create_effect_host() {
    return std::make_unique<EffectHostImpl>();
}

void EffectHost::register_builtins(EffectHost& host) {
    host.register_effect("gaussian_blur", std::make_unique<GaussianBlurEffect>());
    host.register_effect("directional_blur", std::make_unique<DirectionalBlurEffect>());
    host.register_effect("radial_blur", std::make_unique<RadialBlurEffect>());
    host.register_effect("drop_shadow", std::make_unique<DropShadowEffect>());
    host.register_effect("glow", std::make_unique<GlowEffect>());
    host.register_effect("levels", std::make_unique<LevelsEffect>());
    host.register_effect("curves", std::make_unique<CurvesEffect>());
    host.register_effect("fill", std::make_unique<FillEffect>());
    host.register_effect("tint", std::make_unique<TintEffect>());
    host.register_effect("hue_saturation", std::make_unique<HueSaturationEffect>());
    host.register_effect("color_balance", std::make_unique<ColorBalanceEffect>());
    host.register_effect("lut", std::make_unique<LUTEffect>());
    host.register_effect("lut3d_cube", std::make_unique<Lut3DCubeEffect>());
    host.register_effect("chromatic_aberration", std::make_unique<ChromaticAberrationEffect>());
    host.register_effect("vignette", std::make_unique<VignetteEffect>());
    host.register_effect("particle_emitter", std::make_unique<ParticleEmitterEffect>());
    host.register_effect("displacement_map", std::make_unique<DisplacementMapEffect>());
    host.register_effect("chroma_key", std::make_unique<ChromaKeyEffect>());
    host.register_effect("light_wrap", std::make_unique<LightWrapEffect>());
    host.register_effect("matte_refinement", std::make_unique<MatteRefinementEffect>());
    host.register_effect("vector_blur", std::make_unique<VectorBlurEffect>());
}

} // namespace tachyon::renderer2d
