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
LinearRGBA blend_additive(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_soft_light(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_darken(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_lighten(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_color_dodge(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_color_burn(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_hard_light(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_difference(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_exclusion(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_linear_dodge(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_subtract(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_divide(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_linear_burn(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_vivid_light(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_linear_light(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_pin_light(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_hard_mix(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_hue(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_saturation(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_color(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_luminosity(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_darker_color(LinearRGBA src, LinearRGBA dst);
LinearRGBA blend_lighter_color(LinearRGBA src, LinearRGBA dst);

} // namespace tachyon::color
