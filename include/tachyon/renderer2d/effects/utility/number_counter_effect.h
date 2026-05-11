#pragma once

#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/effect_params.h"
#include <cstdio>
#include <string>

namespace tachyon::renderer2d {

/**
 * @brief Formats a number counter based on time and rate.
 * 
 * This is a utility function for expressions and effects to generate
 * formatted number strings from time and rate values.
 * 
 * @param time_seconds Current time in seconds
 * @param rate Frames per second (or multiplier)
 * @param format printf-style format string (default: "%.0f")
 * @return Formatted number string
 */
inline std::string format_number_counter(double time_seconds, double rate, const std::string& format = "%.0f") {
    double value = time_seconds * rate;
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), format.c_str(), value);
    return buffer;
}

class NumberCounterEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

} // namespace tachyon::renderer2d
