#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/effects/color/color_types.h"

namespace tachyon::renderer2d {

/**
 * @brief Composites a source surface onto a destination surface with an offset.
 */
void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy);

/**
 * @brief Converts a standard RGBA color to a premultiplied pixel.
 */
PremultipliedPixel to_premultiplied(Color color);

/**
 * @brief Converts a premultiplied pixel back to a standard RGBA color.
 */
Color from_premultiplied(const PremultipliedPixel& px);

} // namespace tachyon::renderer2d
