#include "tachyon/renderer2d/effects/utility/number_counter_effect.h"
#include "tachyon/renderer2d/effects/core/effect_common.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"
#include <iostream>

namespace tachyon::renderer2d {

SurfaceRGBA NumberCounterEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    // Get parameters
    double time_seconds = get_scalar(params, "time", 0.0);
    double rate = get_scalar(params, "rate", 30.0); // default 30 fps
    std::string format = get_string(params, "format", "%.0f");
    
    // Format the number
    std::string text = format_number_counter(time_seconds, rate, format);
    
    // Render text to surface using font system
    // Note: Full integration with the renderer2d text pipeline is pending.
    return input;
}

} // namespace tachyon::renderer2d
