#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_stylize_effect_implementations() {
    std::vector<EffectImplementation> implementations;

    // Drop Shadow
    implementations.push_back({
        "tachyon.effect.stylize.drop_shadow",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            DropShadowEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Glow
    implementations.push_back({
        "tachyon.effect.stylize.glow",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GlowEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Motion Blur 2D
    implementations.push_back({
        "tachyon.effect.stylize.motion_blur_2d",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            MotionBlur2DEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Keying (Chroma Key)
    implementations.push_back({
        "tachyon.effect.keying.chroma_key",
        [](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ChromaKeyEffect effect;
            output = effect.apply(input, params);
        }
    });

    return implementations;
}

} // namespace tachyon::renderer2d
