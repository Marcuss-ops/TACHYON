#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

std::vector<EffectDescriptor> get_color_effect_descriptors() {
    std::vector<EffectDescriptor> descriptors;

    // Levels
    descriptors.push_back({
        "tachyon.effect.color.levels",
        {"tachyon.effect.color.levels", "Levels", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"input_black", "Input Black", "Input black point", 0.0, 0.0, 255.0},
            {"input_white", "Input White", "Input white point", 255.0, 0.0, 255.0},
            {"gamma", "Gamma", "Midtone adjustment", 1.0, 0.1, 10.0},
            {"output_black", "Output Black", "Output black point", 0.0, 0.0, 255.0},
            {"output_white", "Output White", "Output white point", 255.0, 0.0, 255.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LevelsEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Curves
    descriptors.push_back({
        "tachyon.effect.color.curves",
        {"tachyon.effect.color.curves", "Curves", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"curve_points", "Curve Points", "JSON array of curve control points", "[]"}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            CurvesEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Fill
    descriptors.push_back({
        "tachyon.effect.color.fill",
        {"tachyon.effect.color.fill", "Fill", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"color", "Color", "Fill color", ColorSpec{255, 255, 255, 255}},
            {"blend_mode", "Blend Mode", "How to blend fill", "normal"}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            FillEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Tint
    descriptors.push_back({
        "tachyon.effect.color.tint",
        {"tachyon.effect.color.tint", "Tint", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"amount", "Amount", "Tint intensity", 1.0, 0.0, 1.0},
            {"color", "Color", "Tint color", ColorSpec{255, 128, 0, 255}}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            TintEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Hue/Saturation
    descriptors.push_back({
        "tachyon.effect.color.hue_saturation",
        {"tachyon.effect.color.hue_saturation", "Hue/Saturation", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"hue_shift", "Hue Shift", "Hue rotation in degrees", 0.0, -180.0, 180.0},
            {"saturation", "Saturation", "Saturation adjustment", 0.0, -100.0, 100.0},
            {"lightness", "Lightness", "Lightness adjustment", 0.0, -100.0, 100.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            HueSaturationEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Color Balance
    descriptors.push_back({
        "tachyon.effect.color.balance",
        {"tachyon.effect.color.balance", "Color Balance", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"shadows_r", "Shadows R", "Red in shadows", 0.0, -100.0, 100.0},
            {"shadows_g", "Shadows G", "Green in shadows", 0.0, -100.0, 100.0},
            {"shadows_b", "Shadows B", "Blue in shadows", 0.0, -100.0, 100.0},
            {"midtones_r", "Midtones R", "Red in midtones", 0.0, -100.0, 100.0},
            {"midtones_g", "Midtones G", "Green in midtones", 0.0, -100.0, 100.0},
            {"midtones_b", "Midtones B", "Blue in midtones", 0.0, -100.0, 100.0},
            {"highlights_r", "Highlights R", "Red in highlights", 0.0, -100.0, 100.0},
            {"highlights_g", "Highlights G", "Green in highlights", 0.0, -100.0, 100.0},
            {"highlights_b", "Highlights B", "Blue in highlights", 0.0, -100.0, 100.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ColorBalanceEffect effect;
            output = effect.apply(input, params);
        }
    });

    // LUT
    descriptors.push_back({
        "tachyon.effect.color.lut",
        {"tachyon.effect.color.lut", "LUT", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to LUT file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            LUTEffect effect;
            output = effect.apply(input, params);
        }
    });

    // 3D LUT (.cube)
    descriptors.push_back({
        "tachyon.effect.color.lut3d",
        {"tachyon.effect.color.lut3d", "3D LUT (.cube)", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to .cube file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            Lut3DCubeEffect effect;
            output = effect.apply(input, params);
        }
    });

    return descriptors;
}

} // namespace tachyon::renderer2d
