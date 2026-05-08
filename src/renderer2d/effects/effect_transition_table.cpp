#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectDescriptor> get_transition_effect_descriptors(const tachyon::TransitionRegistry& transition_registry) {
    std::vector<EffectDescriptor> descriptors;

    
    descriptors.push_back({
        "tachyon.effect.transition.glsl",
        {"tachyon.effect.transition.glsl", "GLSL Transition", "GPU-accelerated GLSL transition kernel.", "effect.transition", {"glsl", "transition", "gpu"}},
        registry::ParameterSchema({
            {"progress", "Progress", "Transition progress", 0.0, 0.0, 1.0},
            {"transition_id", "Transition", "ID of the GLSL transition", "fade"}
        }),
        [&transition_registry](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GlslTransitionEffect effect(transition_registry);
            output = effect.apply(input, params);
        },
        {{"transition_to", "transition_to_layer_id", "transition", true}}
    });

    return descriptors;
}

std::vector<presets::EffectPresetSpec> get_transition_effect_preset_specs() {
    std::vector<presets::EffectPresetSpec> specs;

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

} // namespace tachyon::renderer2d
