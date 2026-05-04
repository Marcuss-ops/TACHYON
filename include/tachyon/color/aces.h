#pragma once

#include "tachyon/color/color_space.h"

namespace tachyon::color {

/**
 * ACES filmic tone mapping operator.
 */
LinearRGBA aces_filmic(LinearRGBA c);

/**
 * ACES color space conversion (AP0 to AP1).
 */
LinearRGBA aces_ap0_to_ap1(LinearRGBA ap0);

/**
 * ACES color space conversion (AP1 to AP0).
 */
LinearRGBA aces_ap1_to_ap0(LinearRGBA ap1);

} // namespace tachyon::color
