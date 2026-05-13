#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/effects/color/color_types.h"

namespace tachyon::renderer2d {

// Porter-Duff "over" composite, straight alpha.
void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy);

// Tiled version: clips the source ROI once, then processes in tile_w x tile_h blocks
// for better cache locality. Replaces composite_with_offset in hot paths.
void composite_with_offset_tiled(SurfaceRGBA& dst, const SurfaceRGBA& src,
                                  int ox, int oy,
                                  int tile_w = 128, int tile_h = 64);

/**
 * @brief Converts a standard RGBA color to a premultiplied pixel.
 */
PremultipliedPixel to_premultiplied(Color color);

/**
 * @brief Converts a premultiplied pixel back to a standard RGBA color.
 */
Color from_premultiplied(const PremultipliedPixel& px);

} // namespace tachyon::renderer2d
