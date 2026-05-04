#pragma once

#include "tachyon/color/color_space.h"

namespace tachyon::color {

/**
 * Transfer function types.
 */
enum class TransferFunction {
    sRGB,
    Linear,
    Rec709,
    ACES,
    Gamma22
};

/**
 * Applies transfer function to linear color.
 */
SRGBA apply_transfer(LinearRGBA linear, TransferFunction func);

/**
 * Removes transfer function from non-linear color.
 */
LinearRGBA remove_transfer(LinearRGBA non_linear, TransferFunction func);

} // namespace tachyon::color
