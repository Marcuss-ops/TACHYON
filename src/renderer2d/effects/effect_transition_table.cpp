#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

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

} // namespace tachyon::renderer2d
