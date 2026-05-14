#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_generator_effect_implementations() {
    std::vector<EffectImplementation> implementations;

    // Light Leak
    implementations.push_back({
        "tachyon.effect.generator.light_leak",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LightLeakEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Particle Emitter
    implementations.push_back({
        "tachyon.effect.generator.particle_emitter",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ParticleEmitterEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Lens Flare
    implementations.push_back({
        "tachyon.effect.generator.lens_flare",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LensFlareEffect effect;
            output = effect.apply(input, params);
        }
    });

    return implementations;
}

} // namespace tachyon::renderer2d
