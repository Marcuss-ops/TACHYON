#pragma once

#include <string>

namespace tachyon::core::media {

/**
 * @brief Utilities for generating FFmpeg filter strings for transitions.
 */
class TransitionFilters {
public:
    static std::string generate_fade_in(double duration);
    static std::string generate_fade_out(double start_time, double duration);
};

} // namespace tachyon::core::media
