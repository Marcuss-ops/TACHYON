#include "tachyon/renderer2d/raster/sdf_rasterizer.h"
#include "tachyon/core/math/vector2.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

static float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Framebuffer SDFShapeRasterizer::rasterize(const spec::ShapeSpec& shape_spec, int width, int height) {
    Framebuffer fb(width, height);
    if (width <= 0 || height <= 0) return fb;

    // For now, implement SDF for rectangle only
    if (shape_spec.type != "rectangle" && shape_spec.type != "rounded_rect") {
        // Fallback: fill with solid color (no SDF)
        for (uint32_t y = 0; y < static_cast<uint32_t>(height); ++y) {
            for (uint32_t x = 0; x < static_cast<uint32_t>(width); ++x) {
                fb.pixel(x, y) = Color{255, 255, 255, 255};
            }
        }
        return fb;
    }

    float rect_x = shape_spec.x;
    float rect_y = shape_spec.y;
    float rect_w = shape_spec.width;
    float rect_h = shape_spec.height;
    float radius = shape_spec.radius;

    // SDF for rounded rectangle
    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            float fx = static_cast<float>(px);
            float fy = static_cast<float>(py);

            // Distance to rectangle boundary
            float dx = std::abs(fx - (rect_x + rect_w * 0.5f)) - rect_w * 0.5f;
            float dy = std::abs(fy - (rect_y + rect_h * 0.5f)) - rect_h * 0.5f;

            float dist;
            if (radius > 0.0f && dx < 0.0f && dy < 0.0f) {
                // Inside corner region: distance to rounded corner
                float corner_dx = dx + radius;
                float corner_dy = dy + radius;
                float corner_dist = std::sqrt(corner_dx * corner_dx + corner_dy * corner_dy) - radius;
                dist = corner_dist;
            } else {
                // Outside or edge region: max of dx, dy
                dist = std::max(dx, dy);
            }

            // Apply threshold (smoothstep)
            float threshold = shape_spec.sdf_threshold; // default 0.5
            float alpha = 1.0f - smoothstep(threshold - 0.5f, threshold + 0.5f, dist);
            alpha = std::clamp(alpha, 0.0f, 1.0f);

            Color c = shape_spec.fill_color;
            c.a = static_cast<uint8_t>(alpha * 255.0f);
            fb.pixel(static_cast<uint32_t>(fx), static_cast<uint32_t>(fy)) = c;
        }
    }

    return fb;
}

} // namespace tachyon::renderer2d
