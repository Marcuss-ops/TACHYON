#pragma once

#include <string>
#include <vector>
#include <utility>

namespace tachyon::audio {

/**
 * @brief Utility for building FFmpeg gate expressions.
 * Derived from ruststream-core/audio/gate_utils.rs
 */
class GateUtils {
public:
    /**
     * @brief Builds an FFmpeg expression that gates audio based on time ranges.
     * @param ranges Vector of (start, end) pairs in seconds.
     * @param inverted If true, audio is muted OUTSIDE the ranges. If false, muted INSIDE.
     * @return An FFmpeg expression string.
     */
    static std::string build_gate_expr_from_ranges(
        const std::vector<std::pair<double, double>>& ranges,
        bool inverted = false);

    /**
     * @brief Builds an expression that only allows audio during an initial intro.
     * @param duration Duration of the intro in seconds.
     * @return An FFmpeg expression string.
     */
    static std::string build_intro_only_gate_expr(double duration);
};

} // namespace tachyon::audio
