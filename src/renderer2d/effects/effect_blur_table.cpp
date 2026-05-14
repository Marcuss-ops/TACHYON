#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_blur_effect_implementations() {
    std::vector<EffectImplementation> implementations;

    // Gaussian Blur
    implementations.push_back({
        "tachyon.effect.blur.gaussian",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GaussianBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Directional Blur
    implementations.push_back({
        "tachyon.effect.blur.directional",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            DirectionalBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Radial Blur
    implementations.push_back({
        "tachyon.effect.blur.radial",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            RadialBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Vector Blur
    implementations.push_back({
        "tachyon.effect.blur.vector",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            VectorBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Motion Blur 2D
    implementations.push_back({
        "tachyon.effect.blur.motion_2d",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            MotionBlur2DEffect effect;
            output = effect.apply(input, params);
        }
    });

    return implementations;
}

} // namespace tachyon::renderer2d
