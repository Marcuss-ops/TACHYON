#include "tachyon/presets/effects/effect_specs.h"
#include <vector>

namespace tachyon::presets {

std::vector<EffectKindSpec> get_color_effect_kind_specs() {
    std::vector<EffectKindSpec> specs;

    // Levels
    specs.push_back({
        "tachyon.effect.color.levels",
        {"tachyon.effect.color.levels", "Levels", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"input_black", "Input Black", "Input black point", 0.0, 0.0, 255.0},
            {"input_white", "Input White", "Input white point", 255.0, 0.0, 255.0},
            {"gamma", "Gamma", "Midtone adjustment", 1.0, 0.1, 10.0},
            {"output_black", "Output Black", "Output black point", 0.0, 0.0, 255.0},
            {"output_white", "Output White", "Output white point", 255.0, 0.0, 255.0}
        })
    });

    // Curves
    specs.push_back({
        "tachyon.effect.color.curves",
        {"tachyon.effect.color.curves", "Curves", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"curve_points", "Curve Points", "JSON array of curve control points", "[]"}
        })
    });

    // Fill
    specs.push_back({
        "tachyon.effect.color.fill",
        {"tachyon.effect.color.fill", "Fill", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"color", "Color", "Fill color", ColorSpec{255, 255, 255, 255}},
            {"blend_mode", "Blend Mode", "How to blend fill", "normal"}
        })
    });

    // Tint
    specs.push_back({
        "tachyon.effect.color.tint",
        {"tachyon.effect.color.tint", "Tint", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"amount", "Amount", "Tint intensity", 1.0, 0.0, 1.0},
            {"color", "Color", "Tint color", ColorSpec{255, 128, 0, 255}}
        })
    });

    // Hue/Saturation
    specs.push_back({
        "tachyon.effect.color.hue_saturation",
        {"tachyon.effect.color.hue_saturation", "Hue/Saturation", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"hue_shift", "Hue Shift", "Hue rotation in degrees", 0.0, -180.0, 180.0},
            {"saturation", "Saturation", "Saturation adjustment", 0.0, -100.0, 100.0},
            {"lightness", "Lightness", "Lightness adjustment", 0.0, -100.0, 100.0}
        })
    });

    // Color Balance
    specs.push_back({
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
        })
    });

    // LUT
    specs.push_back({
        "tachyon.effect.color.lut",
        {"tachyon.effect.color.lut", "LUT", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to LUT file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        })
    });

    // LUT Cube (.cube)
    specs.push_back({
        "tachyon.effect.color.lut_cube",
        {"tachyon.effect.color.lut_cube", "LUT Cube", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to .cube file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        })
    });

    // Chroma Key
    specs.push_back({
        "tachyon.effect.color.chroma_key",
        {"tachyon.effect.color.chroma_key", "Chroma Key", "Professional keying effect.", "effect.color", {"keying"}},
        registry::ParameterSchema({
            {"key_color", "Key Color", "Color to remove", ColorSpec{0, 255, 0, 255}},
            {"similarity", "Similarity", "Color similarity threshold", 0.1, 0.0, 1.0},
            {"smoothness", "Smoothness", "Edge smoothness", 0.1, 0.0, 1.0}
        })
    });

    // Light Wrap
    specs.push_back({
        "tachyon.effect.color.light_wrap",
        {"tachyon.effect.color.light_wrap", "Light Wrap", "Professional keying effect.", "effect.color", {"keying"}},
        registry::ParameterSchema({
            {"width", "Width", "Wrap width", 10.0, 0.0, 100.0},
            {"intensity", "Intensity", "Wrap intensity", 1.0, 0.0, 1.0}
        })
    });

    // Matte Refinement
    specs.push_back({
        "tachyon.effect.color.matte_refinement",
        {"tachyon.effect.color.matte_refinement", "Matte Refinement", "Professional keying effect.", "effect.color", {"keying"}},
        registry::ParameterSchema({
            {"choke", "Choke/Spread", "Choke (negative) or spread (positive)", 0.0, -100.0, 100.0},
            {"feather", "Feather", "Edge feathering", 0.0, 0.0, 100.0}
        })
    });

    return specs;
}

std::vector<EffectPresetSpec> get_color_effect_preset_specs() {
    std::vector<EffectPresetSpec> specs;

    // Levels Preset
    specs.push_back({
        "tachyon.effect.color.levels",
        {"tachyon.effect.color.levels", "Levels", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"input_black", "Input Black", "Input black point", 0.0, 0.0, 255.0},
            {"input_white", "Input White", "Input white point", 255.0, 0.0, 255.0},
            {"gamma", "Gamma", "Midtone adjustment", 1.0, 0.1, 10.0},
            {"output_black", "Output Black", "Output black point", 0.0, 0.0, 255.0},
            {"output_white", "Output White", "Output white point", 255.0, 0.0, 255.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.levels";
            effect.scalars["input_black"] = p.get_or<double>("input_black", 0.0);
            effect.scalars["input_white"] = p.get_or<double>("input_white", 255.0);
            effect.scalars["gamma"] = p.get_or<double>("gamma", 1.0);
            effect.scalars["output_black"] = p.get_or<double>("output_black", 0.0);
            effect.scalars["output_white"] = p.get_or<double>("output_white", 255.0);
            return effect;
        }
    });

    // Curves Preset
    specs.push_back({
        "tachyon.effect.color.curves",
        {"tachyon.effect.color.curves", "Curves", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"curve_points", "Curve Points", "JSON array of curve control points", "[]"}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.curves";
            effect.strings["curve_points"] = p.get_or<std::string>("curve_points", "[]");
            return effect;
        }
    });

    // Fill Preset
    specs.push_back({
        "tachyon.effect.color.fill",
        {"tachyon.effect.color.fill", "Fill", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"color", "Color", "Fill color", ColorSpec{255, 255, 255, 255}},
            {"blend_mode", "Blend Mode", "How to blend fill", "normal"}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.fill";
            effect.colors["color"] = p.get_or<ColorSpec>("color", ColorSpec{255, 255, 255, 255});
            effect.strings["blend_mode"] = p.get_or<std::string>("blend_mode", "normal");
            return effect;
        }
    });

    // Tint Preset
    specs.push_back({
        "tachyon.effect.color.tint",
        {"tachyon.effect.color.tint", "Tint", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"amount", "Amount", "Tint intensity", 1.0, 0.0, 1.0},
            {"color", "Color", "Tint color", ColorSpec{255, 128, 0, 255}}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.tint";
            effect.scalars["amount"] = p.get_or<double>("amount", 1.0);
            effect.colors["color"] = p.get_or<ColorSpec>("color", ColorSpec{255, 128, 0, 255});
            return effect;
        }
    });

    // Hue/Saturation Preset
    specs.push_back({
        "tachyon.effect.color.hue_saturation",
        {"tachyon.effect.color.hue_saturation", "Hue/Saturation", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"hue_shift", "Hue Shift", "Hue rotation in degrees", 0.0, -180.0, 180.0},
            {"saturation", "Saturation", "Saturation adjustment", 0.0, -100.0, 100.0},
            {"lightness", "Lightness", "Lightness adjustment", 0.0, -100.0, 100.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.hue_saturation";
            effect.scalars["hue_shift"] = p.get_or<double>("hue_shift", 0.0);
            effect.scalars["saturation"] = p.get_or<double>("saturation", 0.0);
            effect.scalars["lightness"] = p.get_or<double>("lightness", 0.0);
            return effect;
        }
    });

    // Color Balance Preset
    specs.push_back({
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
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.balance";
            effect.scalars["shadows_r"] = p.get_or<double>("shadows_r", 0.0);
            effect.scalars["shadows_g"] = p.get_or<double>("shadows_g", 0.0);
            effect.scalars["shadows_b"] = p.get_or<double>("shadows_b", 0.0);
            effect.scalars["midtones_r"] = p.get_or<double>("midtones_r", 0.0);
            effect.scalars["midtones_g"] = p.get_or<double>("midtones_g", 0.0);
            effect.scalars["midtones_b"] = p.get_or<double>("midtones_b", 0.0);
            effect.scalars["highlights_r"] = p.get_or<double>("highlights_r", 0.0);
            effect.scalars["highlights_g"] = p.get_or<double>("highlights_g", 0.0);
            effect.scalars["highlights_b"] = p.get_or<double>("highlights_b", 0.0);
            return effect;
        }
    });

    // LUT Preset
    specs.push_back({
        "tachyon.effect.color.lut",
        {"tachyon.effect.color.lut", "LUT", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to LUT file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.lut";
            effect.strings["lut_file"] = p.get_or<std::string>("lut_file", "");
            effect.scalars["intensity"] = p.get_or<double>("intensity", 1.0);
            return effect;
        }
    });

    // LUT Cube Preset
    specs.push_back({
        "tachyon.effect.color.lut_cube",
        {"tachyon.effect.color.lut_cube", "LUT Cube", "Professional color effect.", "effect.color", {"color"}},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to .cube file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.lut";
            effect.strings["lut_file"] = p.get_or<std::string>("lut_file", "");
            effect.scalars["intensity"] = p.get_or<double>("intensity", 1.0);
            return effect;
        }
    });

    // Chroma Key Preset
    specs.push_back({
        "tachyon.effect.color.chroma_key",
        {"tachyon.effect.color.chroma_key", "Chroma Key", "Professional keying effect.", "effect.color", {"keying"}},
        registry::ParameterSchema({
            {"key_color", "Key Color", "Color to remove", ColorSpec{0, 255, 0, 255}},
            {"similarity", "Similarity", "Color similarity threshold", 0.1, 0.0, 1.0},
            {"smoothness", "Smoothness", "Edge smoothness", 0.1, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.chroma_key";
            effect.colors["key_color"] = p.get_or<ColorSpec>("key_color", ColorSpec{0, 255, 0, 255});
            effect.scalars["similarity"] = p.get_or<double>("similarity", 0.1);
            effect.scalars["smoothness"] = p.get_or<double>("smoothness", 0.1);
            return effect;
        }
    });

    // Light Wrap Preset
    specs.push_back({
        "tachyon.effect.color.light_wrap",
        {"tachyon.effect.color.light_wrap", "Light Wrap", "Professional keying effect.", "effect.color", {"keying"}},
        registry::ParameterSchema({
            {"width", "Width", "Wrap width", 10.0, 0.0, 100.0},
            {"intensity", "Intensity", "Wrap intensity", 1.0, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.light_wrap";
            effect.scalars["width"] = p.get_or<double>("width", 10.0);
            effect.scalars["intensity"] = p.get_or<double>("intensity", 1.0);
            return effect;
        }
    });

    // Matte Refinement Preset
    specs.push_back({
        "tachyon.effect.color.matte_refinement",
        {"tachyon.effect.color.matte_refinement", "Matte Refinement", "Professional keying effect.", "effect.color", {"keying"}},
        registry::ParameterSchema({
            {"choke", "Choke/Spread", "Choke (negative) or spread (positive)", 0.0, -100.0, 100.0},
            {"feather", "Feather", "Edge feathering", 0.0, 0.0, 100.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.color.matte_refinement";
            effect.scalars["choke"] = p.get_or<double>("choke", 0.0);
            effect.scalars["feather"] = p.get_or<double>("feather", 0.0);
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::presets
