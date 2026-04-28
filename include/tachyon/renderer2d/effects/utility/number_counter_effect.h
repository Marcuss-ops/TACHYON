#pragma once

#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/effect_params.h"
#include <string>
#include <cstdio>

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

/**
 * @brief Formats current date/time as string.
 * 
 * @param format strftime-style format string (default: "%Y-%m-%d %H:%M:%S")
 * @return Formatted date string
 */
inline std::string format_current_date(const std::string& format = "%Y-%m-%d %H:%M:%S") {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    char buffer[256];
    std::strftime(buffer, sizeof(buffer), format.c_str(), std::localtime(&time_t_now));
    return buffer;
}

class NumberCounterEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

} // namespace tachyon::renderer2d
