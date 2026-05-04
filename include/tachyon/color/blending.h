#pragma once

#include "tachyon/color/color_space.h"

namespace tachyon::color {

/**
 * Normal blend mode.
 */
LinearRGBA blend_normal(LinearRGBA src, LinearRGBA dst);

/**
 * Multiply blend mode.
 */
LinearRGBA blend_multiply(LinearRGBA src, LinearRGBA dst);

/**
 * Screen blend mode.
 */
LinearRGBA blend_screen(LinearRGBA src, LinearRGBA dst);

/**
 * Overlay blend mode.
 */
LinearRGBA blend_overlay(LinearRGBA src, LinearRGBA dst);

} // namespace tachyon::color
