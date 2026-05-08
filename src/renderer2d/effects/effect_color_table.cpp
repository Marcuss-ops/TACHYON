#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_color_effect_implementations() {
    std::vector<EffectImplementation> implementations;

    // Levels
    implementations.push_back({
        "tachyon.effect.color.levels",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LevelsEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Curves
    implementations.push_back({
        "tachyon.effect.color.curves",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            CurvesEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Fill
    implementations.push_back({
        "tachyon.effect.color.fill",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            FillEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Tint
    implementations.push_back({
        "tachyon.effect.color.tint",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            TintEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Hue/Saturation
    implementations.push_back({
        "tachyon.effect.color.hue_saturation",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            HueSaturationEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Color Balance
    implementations.push_back({
        "tachyon.effect.color.balance",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ColorBalanceEffect effect;
            output = effect.apply(input, params);
        }
    });

    // LUT
    implementations.push_back({
        "tachyon.effect.color.lut",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LUTEffect effect;
            output = effect.apply(input, params);
        }
    });

    // 3D LUT (.cube)
    implementations.push_back({
        "tachyon.effect.color.lut3d",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            Lut3DCubeEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Chroma Key
    implementations.push_back({
        "tachyon.effect.color.chroma_key",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ChromaKeyEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Light Wrap
    implementations.push_back({
        "tachyon.effect.color.light_wrap",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LightWrapEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Matte Refinement
    implementations.push_back({
        "tachyon.effect.color.matte_refinement",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            MatteRefinementEffect effect;
            output = effect.apply(input, params);
        }
    });

    return implementations;
}

} // namespace tachyon::renderer2d
