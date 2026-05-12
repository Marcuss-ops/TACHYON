#pragma once

#include <array>
#include <type_traits>

namespace tachyon::color {

/**
 * Linear RGBA color (linear color space).
 * Aligned to 16 bytes for SSE/AVX safety.
 */
struct alignas(16) LinearRGBA {
    float r{0.0f}, g{0.0f}, b{0.0f}, a{1.0f};
};

static_assert(sizeof(LinearRGBA) == 16, "LinearRGBA must be exactly 16 bytes");
static_assert(alignof(LinearRGBA) == 16, "LinearRGBA must be 16-byte aligned");
static_assert(std::is_standard_layout_v<LinearRGBA>, "LinearRGBA must have standard layout for SIMD");

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
