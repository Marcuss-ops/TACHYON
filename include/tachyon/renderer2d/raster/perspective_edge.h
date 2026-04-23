#pragma once

#include "tachyon/renderer2d/raster/perspective_rasterizer.h"
#include <cmath>

namespace tachyon::renderer2d::raster {

/// Edge structure for perspective-correct rasterization.
/// Used for scanline interpolation with perspective correction.
struct PerspectiveEdge {
    float x, step_x;
    float u_w, step_u_w;
    float v_w, step_v_w;
    float one_w, step_one_w;
    int y_start, y_end;

    PerspectiveEdge(const Vertex3D& v0, const Vertex3D& v1) {
        y_start = static_cast<int>(std::ceil(v0.position.y));
        y_end = static_cast<int>(std::ceil(v1.position.y));
        float dy = v1.position.y - v0.position.y;
        if (dy > 0.0f) {
            float prestep = static_cast<float>(y_start) - v0.position.y;
            step_x = (v1.position.x - v0.position.x) / dy;
            x = v0.position.x + step_x * prestep;

            const float u0_w = v0.uv.x * v0.position.z;
            const float v0_w = v0.uv.y * v0.position.z;
            const float u1_w = v1.uv.x * v1.position.z;
            const float v1_w = v1.uv.y * v1.position.z;

            step_u_w = (u1_w - u0_w) / dy;
            u_w = u0_w + step_u_w * prestep;

            step_v_w = (v1_w - v0_w) / dy;
            v_w = v0_w + step_v_w * prestep;

            step_one_w = (v1.position.z - v0.position.z) / dy;
            one_w = v0.position.z + step_one_w * prestep;
        }
    }

    void step() {
        x += step_x;
        u_w += step_u_w;
        v_w += step_v_w;
        one_w += step_one_w;
    }
};

} // namespace tachyon::renderer2d::raster