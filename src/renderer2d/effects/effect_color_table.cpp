#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_color_effect_implementations() {
    return {
        make_surface_effect<LevelsEffect>("tachyon.effect.color.levels"),
        make_surface_effect<CurvesEffect>("tachyon.effect.color.curves"),
        make_surface_effect<FillEffect>("tachyon.effect.color.fill"),
        make_surface_effect<TintEffect>("tachyon.effect.color.tint"),
        make_surface_effect<HueSaturationEffect>("tachyon.effect.color.hue_saturation"),
        make_surface_effect<ColorBalanceEffect>("tachyon.effect.color.balance"),
        make_surface_effect<LUTEffect>("tachyon.effect.color.lut"),
        make_surface_effect<ChromaKeyEffect>("tachyon.effect.color.chroma_key"),
        make_surface_effect<LightWrapEffect>("tachyon.effect.color.light_wrap"),
        make_surface_effect<MatteRefinementEffect>("tachyon.effect.color.matte_refinement")
    };
}

} // namespace tachyon::renderer2d
