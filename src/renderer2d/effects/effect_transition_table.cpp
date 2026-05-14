#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_transition_effect_implementations(const tachyon::TransitionRegistry& transition_registry) {
    std::vector<EffectImplementation> implementations;

    implementations.push_back({
        "tachyon.effect.transition.glsl",
        // apply_fn (captures transition_registry by reference)
        [&transition_registry](const EvaluatedEffect&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GlslTransitionEffect effect(transition_registry);
            output = effect.apply(input, params);
        }
    });

    return implementations;
}

} // namespace tachyon::renderer2d
