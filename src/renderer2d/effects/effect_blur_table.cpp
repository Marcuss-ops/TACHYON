#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_blur_effect_implementations() {
    return {
        make_surface_effect<GaussianBlurEffect>("tachyon.effect.blur.gaussian"),
        make_surface_effect<DirectionalBlurEffect>("tachyon.effect.blur.directional"),
        make_surface_effect<RadialBlurEffect>("tachyon.effect.blur.radial"),
        make_surface_effect<VectorBlurEffect>("tachyon.effect.blur.vector"),
        make_surface_effect<MotionBlur2DEffect>("tachyon.effect.blur.motion_2d")
    };
}

} // namespace tachyon::renderer2d
