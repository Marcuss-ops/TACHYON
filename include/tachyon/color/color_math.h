#pragma once
#include "tachyon/color/color_space.h"
#include <algorithm>
#include <cmath>

namespace tachyon::color {

/**
 * @brief Rec.709 linear luminance weights.
 * Used for sRGB linear color components.
 */
inline float luminance_rec709(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

inline float luminance_rec709(const LinearRGBA& c) {
    return luminance_rec709(c.r, c.g, c.b);
}

/**
 * @brief Rec.601 luma weights.
 * Typically used for non-linear (gamma-encoded) components or legacy effects.
 */
inline float luma_rec601(float r, float g, float b) {
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

inline float luma_rec601(const LinearRGBA& c) {
    return luma_rec601(c.r, c.g, c.b);
}

/**
 * @brief Converts RGB to HSL.
 * All parameters are in [0, 1] range.
 */
void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l);

/**
 * @brief Converts HSL to RGB.
 * All parameters are in [0, 1] range.
 */
LinearRGBA hsl_to_rgb(float h, float s, float l, float a = 1.0f);

} // namespace tachyon::color
