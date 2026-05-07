#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"

#include <array>

namespace tachyon::renderer2d {

namespace {

static const std::array<EffectBuiltinSpec, 1> kTransitionEffects = {{
    {
        "tachyon.effect.transition.glsl",
        "GLSL Transition",
        "transition",
        "GPU-accelerated GLSL transition kernel.",
        {"glsl", "transition", "gpu"},
        registry::ParameterSchema({
            {"progress", "Progress", "Transition progress", 0.0, 0.0, 1.0},
            {"transition_id", "Transition", "ID of the GLSL transition", "fade"}
        }),
        make_effect_factory<GlslTransitionEffect>(),
        {{"transition_to", "transition_to_layer_id", "transition", true}},
        true,
        false
    }
}};

} // namespace

void register_transition_effects(EffectRegistry& registry) {
    init_builtin_transitions();
    for (const auto& spec : kTransitionEffects) {
        register_effect_from_spec(registry, spec);
    }
}

} // namespace tachyon::renderer2d
