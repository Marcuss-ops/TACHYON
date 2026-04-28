#pragma once

#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <vector>

namespace tachyon::renderer2d::raster {

struct Vertex3D {
    math::Vector3 position; // Screen space (x, y) and 1/w depth
    math::Vector2 uv;
    float one_over_w{1.0f};
};

struct PerspectiveWarpQuad {
    Vertex3D v0, v1, v2, v3;
    const SurfaceRGBA* texture{nullptr};
    float opacity{1.0f};
};

class PerspectiveRasterizer {
public:
    /**
     * Rasterizes a quadrilateral with perspective-correct texture mapping.
     */
    static void draw_quad(SurfaceRGBA& surface, const PerspectiveWarpQuad& quad);

private:
    static void draw_triangle(
        SurfaceRGBA& surface,
        const Vertex3D& v0,
        const Vertex3D& v1,
        const Vertex3D& v2,
        const SurfaceRGBA* texture,
        float opacity);
};

} // namespace tachyon::renderer2d::raster

