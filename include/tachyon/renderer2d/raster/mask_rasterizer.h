#pragma once

#include "tachyon/renderer2d/raster/path/fill_rasterizer.h"
#include <vector>
#include <cstdint>

namespace tachyon::renderer2d {

/**
 * @brief High-performance mask rasterizer.
 * 
 * Optimized for alpha-only output, skipping color blending and gradient logic.
 * Used for Roto-masks and Track Mattes.
 */
class MaskRasterizer {
public:
    struct Config {
        WindingRule winding{WindingRule::NonZero};
        float feather{0.0f};
    };

    /**
     * @brief Rasterize a polygon into an alpha buffer.
     * 
     * @param width Width of the buffer.
     * @param height Height of the buffer.
     * @param contours The polygon contours.
     * @param cfg Configuration (winding, etc).
     * @param out_alpha The output buffer (must be width * height).
     */
    static void rasterize(
        int width, int height,
        const std::vector<Contour>& contours,
        const Config& cfg,
        float* out_alpha);
};

} // namespace tachyon::renderer2d
