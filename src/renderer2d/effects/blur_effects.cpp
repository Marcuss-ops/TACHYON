#include "tachyon/renderer2d/effects/effect_common.h"

namespace tachyon::renderer2d {

SurfaceRGBA GaussianBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    return blur_surface(input, get_scalar(params, "blur_radius", 0.0f));
}

} // namespace tachyon::renderer2d
