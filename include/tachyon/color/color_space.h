#pragma once

#include <array>

namespace tachyon::color {

/**
 * Linear RGBA color (linear color space).
 */
struct LinearRGBA {
    float r{0.0f}, g{0.0f}, b{0.0f}, a{1.0f};
};

/**
 * sRGB gamma-encoded color.
 */
struct SRGBA {
    float r{0.0f}, g{0.0f}, b{0.0f}, a{1.0f};
};

/**
 * Converts sRGB to linear RGB.
 */
LinearRGBA srgb_to_linear(SRGBA c);

/**
 * Converts linear RGB to sRGB.
 */
SRGBA linear_to_srgb(LinearRGBA c);

} // namespace tachyon::color
