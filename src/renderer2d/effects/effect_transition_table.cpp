#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

void register_transition_effects(EffectRegistry& registry) {
    EffectDescriptor glsl_desc;
    glsl_desc.id = "tachyon.effect.transition.glsl";
    glsl_desc.metadata = {
        glsl_desc.id,
        "GLSL Transition",
        "GPU-accelerated GLSL transition kernel.",
        "effect.transition",
        {"glsl", "transition", "gpu"}
    };
    glsl_desc.schema = registry::ParameterSchema({
        {"progress", "Progress", "Transition progress", 0.0, 0.0, 1.0},
        {"transition_id", "Transition", "ID of the GLSL transition", "fade"}
    });
    glsl_desc.aux_requirements.push_back({"transition_to", "transition_to_layer_id", "transition", true});
    glsl_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        GlslTransitionEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(glsl_desc));
}

} // namespace tachyon::renderer2d
