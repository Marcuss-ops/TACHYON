#pragma once
#include <algorithm>
#include <limits>

namespace tachyon::scene::builder_detail {

/**
 * @brief Clamps a double value to the range of representable 32-bit signed integers.
 */
inline int clamp_to_int(double value) {
    const double bounded = std::clamp(
        value,
        static_cast<double>(std::numeric_limits<int>::min()),
        static_cast<double>(std::numeric_limits<int>::max()));
    return static_cast<int>(bounded);
}

} // namespace tachyon::scene::builder_detail
