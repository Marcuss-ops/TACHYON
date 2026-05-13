#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <vector>

namespace tachyon::renderer2d::raster {

struct WarpVertex {
    math::Vector2 position; // Screen space (x, y)
    math::Vector2 uv;
    float one_over_w{1.0f};
};

struct PerspectiveWarpQuad {
    WarpVertex v0, v1, v2, v3;
    const SurfaceRGBA* texture{nullptr};
    float opacity{1.0f};
};

class WarpRasterizer {
public:
    /**
     * Rasterizes a quadrilateral with perspective-correct texture mapping.
     */
    static void draw_quad(SurfaceRGBA& surface, const PerspectiveWarpQuad& quad);

private:
    static void draw_triangle(
        SurfaceRGBA& surface,
        const WarpVertex& v0,
        const WarpVertex& v1,
        const WarpVertex& v2,
        const SurfaceRGBA* texture,
        float opacity);
};

} // namespace tachyon::renderer2d::raster
