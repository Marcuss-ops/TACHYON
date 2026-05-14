#include "tachyon/presets/effects/effect_specs.h"
#include <vector>

namespace tachyon::presets {

std::vector<EffectKindSpec> get_stylize_effect_kind_specs() {
    std::vector<EffectKindSpec> specs;

    // Drop Shadow
    specs.push_back({
        "tachyon.effect.stylize.drop_shadow",
        {"tachyon.effect.stylize.drop_shadow", "Drop Shadow", "Adds a soft shadow behind the layer.", "effect.stylize", {"shadow", "drop", "depth"}},
        registry::ParameterSchema({
            {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
            {"offset_x", "Offset X", "Horizontal shift", 4.0},
            {"offset_y", "Offset Y", "Vertical shift", 4.0},
            {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
        })
    });

    // Glow
    specs.push_back({
        "tachyon.effect.stylize.glow",
        {"tachyon.effect.stylize.glow", "Glow", "Adds a luminous halo to bright areas.", "effect.stylize", {"glow", "bloom", "light"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
            {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
            {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
        })
    });

    return specs;
}

std::vector<EffectPresetSpec> get_stylize_effect_preset_specs() {
    std::vector<EffectPresetSpec> specs;

    // Drop Shadow Preset
    specs.push_back({
        "tachyon.effect.stylize.drop_shadow",
        {"tachyon.effect.stylize.drop_shadow", "Drop Shadow", "Adds a soft shadow behind the layer.", "effect.stylize", {"shadow", "drop", "depth"}},
        registry::ParameterSchema({
            {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
            {"offset_x", "Offset X", "Horizontal shift", 4.0},
            {"offset_y", "Offset Y", "Vertical shift", 4.0},
            {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.stylize.drop_shadow";
            effect.params["blur_radius"] = p.get_or<double>("blur_radius", 4.0);
            effect.params["offset_x"] = p.get_or<double>("offset_x", 4.0);
            effect.params["offset_y"] = p.get_or<double>("offset_y", 4.0);
            effect.params["shadow_color"] = p.get_or<ColorSpec>("shadow_color", ColorSpec{0, 0, 0, 160});
            return effect;
        }
    });

    // Glow Preset
    specs.push_back({
        "tachyon.effect.stylize.glow",
        {"tachyon.effect.stylize.glow", "Glow", "Adds a luminous halo to bright areas.", "effect.stylize", {"glow", "bloom", "light"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
            {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
            {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.stylize.glow";
            effect.params["radius"] = p.get_or<double>("radius", 4.0);
            effect.params["strength"] = p.get_or<double>("strength", 1.0);
            effect.params["threshold"] = p.get_or<double>("threshold", 0.0);
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::presets
