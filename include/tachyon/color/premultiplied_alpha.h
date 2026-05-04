#pragma once

#include "tachyon/color/color_space.h"

namespace tachyon::color {

/**
 * Premultiplies alpha into RGB channels.
 */
LinearRGBA premultiply(LinearRGBA c);

/**
 * Unpremultiplies alpha from RGB channels.
 * Returns black with alpha=0 if alpha is 0.
 */
LinearRGBA unpremultiply(LinearRGBA c);

} // namespace tachyon::color
