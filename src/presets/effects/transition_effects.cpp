#include "tachyon/presets/effects/effect_specs.h"
#include <vector>

namespace tachyon::presets {

std::vector<EffectKindSpec> get_transition_effect_kind_specs() {
    std::vector<EffectKindSpec> specs;

    // GLSL Transition
    specs.push_back({
        "tachyon.effect.transition.glsl",
        {"tachyon.effect.transition.glsl", "GLSL Transition", "GPU-accelerated GLSL transition kernel.", "effect.transition", {"glsl", "transition", "gpu"}},
        registry::ParameterSchema({
            {"progress", "Progress", "Transition progress", 0.0, 0.0, 1.0},
            {"transition_id", "Transition", "ID of the GLSL transition", "fade"}
        })
    });

    return specs;
}

std::vector<EffectPresetSpec> get_transition_effect_preset_specs() {
    std::vector<EffectPresetSpec> specs;

    // GLSL Transition Preset
    specs.push_back({
        "tachyon.effect.transition.glsl",
        {"tachyon.effect.transition.glsl", "GLSL Transition", "GPU-accelerated GLSL transition kernel.", "effect.transition", {"glsl", "transition", "gpu"}},
        registry::ParameterSchema({
            {"progress", "Progress", "Transition progress", 0.0, 0.0, 1.0},
            {"transition_id", "Transition", "ID of the GLSL transition", "fade"}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.transition.glsl";
            effect.scalars["progress"] = p.get_or<double>("progress", 0.0);
            effect.strings["transition_id"] = p.get_or<std::string>("transition_id", "fade");
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::presets
