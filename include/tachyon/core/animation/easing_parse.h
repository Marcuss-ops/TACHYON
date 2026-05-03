#pragma once

#include "tachyon/core/animation/easing.h"
#include <string>

namespace tachyon::animation {

/**
 * @brief Parses an easing preset from a string.
 */
EasingPreset parse_easing_preset(const std::string& name);

} // namespace tachyon::animation
