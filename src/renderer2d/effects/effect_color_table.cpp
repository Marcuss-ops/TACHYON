#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/effect_descriptor.h"

namespace tachyon::renderer2d {

namespace {

constexpr std::array<EffectBuiltinSpec, 9> kColorEffects = {{
    {
        "tachyon.effect.color.levels",
        "Levels",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"input_black", "Input Black", "Input black point", 0.0, 0.0, 255.0},
            {"input_white", "Input White", "Input white point", 255.0, 0.0, 255.0},
            {"gamma", "Gamma", "Midtone adjustment", 1.0, 0.1, 10.0},
            {"output_black", "Output Black", "Output black point", 0.0, 0.0, 255.0},
            {"output_white", "Output White", "Output white point", 255.0, 0.0, 255.0}
        }),
        make_effect_factory<LevelsEffect>()
    },
    {
        "tachyon.effect.color.curves",
        "Curves",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"curve_points", "Curve Points", "JSON array of curve control points", "[]"}
        }),
        make_effect_factory<CurvesEffect>()
    },
    {
        "tachyon.effect.color.fill",
        "Fill",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"color", "Color", "Fill color", ColorSpec{255, 255, 255, 255}},
            {"blend_mode", "Blend Mode", "How to blend fill", "normal"}
        }),
        make_effect_factory<FillEffect>()
    },
    {
        "tachyon.effect.color.tint",
        "Tint",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"amount", "Amount", "Tint intensity", 1.0, 0.0, 1.0},
            {"color", "Color", "Tint color", ColorSpec{255, 128, 0, 255}}
        }),
        make_effect_factory<TintEffect>()
    },
    {
        "tachyon.effect.color.hue_saturation",
        "Hue/Saturation",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"hue_shift", "Hue Shift", "Hue rotation in degrees", 0.0, -180.0, 180.0},
            {"saturation", "Saturation", "Saturation adjustment", 0.0, -100.0, 100.0},
            {"lightness", "Lightness", "Lightness adjustment", 0.0, -100.0, 100.0}
        }),
        make_effect_factory<HueSaturationEffect>()
    },
    {
        "tachyon.effect.color.balance",
        "Color Balance",
        "color",
        {},
        {},
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
        make_effect_factory<ColorBalanceEffect>()
    },
    {
        "tachyon.effect.color.lut",
        "LUT",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to LUT file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        make_effect_factory<LUTEffect>()
    },
    {
        "tachyon.effect.color.lut3d",
        "3D LUT (.cube)",
        "color",
        {},
        {},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to .cube file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        make_effect_factory<Lut3DCubeEffect>()
    }
}};

} // namespace

void register_color_effects(EffectRegistry& registry) {
    for (const auto& spec : kColorEffects) {
        register_effect_from_spec(registry, spec);
    }
}

} // namespace tachyon::renderer2d
