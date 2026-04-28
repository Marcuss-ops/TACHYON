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
    
    // For now, output to console (stub until text rendering is integrated)
    std::cout << "NumberCounter: " << text << std::endl;
    
    // TODO: Render text to surface using font system
    // Return input unchanged for now
    return input;
}

} // namespace tachyon::renderer2d
