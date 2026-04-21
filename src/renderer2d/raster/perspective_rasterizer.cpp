#include "tachyon/renderer2d/raster/perspective_rasterizer.h"

#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d::raster {

namespace {

struct Edge {
    float x, step_x;
    float u_w, step_u_w;
    float v_w, step_v_w;
    float one_w, step_one_w;
    int y_start, y_end;

    Edge(const Vertex3D& v0, const Vertex3D& v1) {
        y_start = static_cast<int>(std::ceil(v0.position.y));
        y_end = static_cast<int>(std::ceil(v1.position.y));
        float dy = v1.position.y - v0.position.y;
        if (dy > 0.0f) {
            float prestep = static_cast<float>(y_start) - v0.position.y;
            step_x = (v1.position.x - v0.position.x) / dy;
            x = v0.position.x + step_x * prestep;

            const float u0_w = v0.uv.x * v0.one_over_w;
            const float v0_w = v0.uv.y * v0.one_over_w;
            const float u1_w = v1.uv.x * v1.one_over_w;
            const float v1_w = v1.uv.y * v1.one_over_w;

            step_u_w = (u1_w - u0_w) / dy;
            u_w = u0_w + step_u_w * prestep;

            step_v_w = (v1_w - v0_w) / dy;
            v_w = v0_w + step_v_w * prestep;

            step_one_w = (v1.one_over_w - v0.one_over_w) / dy;
            one_w = v0.one_over_w + step_one_w * prestep;
        }
    }

    void step() {
        x += step_x;
        u_w += step_u_w;
        v_w += step_v_w;
        one_w += step_one_w;
    }
};

void draw_scanline(
    SurfaceRGBA& surface,
    int y,
    const Edge& left,
    const Edge& right,
    const SurfaceRGBA* texture,
    float opacity) {

    int x_start = static_cast<int>(std::ceil(left.x));
    int x_end = static_cast<int>(std::ceil(right.x));
    float dx = right.x - left.x;

    if (dx <= 0.0f) return;

    float step_u_w = (right.u_w - left.u_w) / dx;
    float step_v_w = (right.v_w - left.v_w) / dx;
    float step_one_w = (right.one_w - left.one_w) / dx;

    float prestep = static_cast<float>(x_start) - left.x;
    float u_w = left.u_w + step_u_w * prestep;
    float v_w = left.v_w + step_v_w * prestep;
    float one_w = left.one_w + step_one_w * prestep;

    const std::uint32_t tex_w = texture->width();
    const std::uint32_t tex_h = texture->height();

    for (int x = x_start; x < x_end; ++x) {
        // Perspective correct depth test
        // one_w is 1/z in screen space
        if (surface.test_and_write_depth(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), one_w)) {
            // Perspective divide for UV mapping
            float inv_w = 1.0f / one_w;
            float u = u_w * inv_w;
            float v = v_w * inv_w;

            // Wrap/Clamp UV (assume repeat for now or clamp)
            int tx = std::clamp(static_cast<int>(u * tex_w), 0, static_cast<int>(tex_w - 1));
            int ty = std::clamp(static_cast<int>(v * tex_h), 0, static_cast<int>(tex_h - 1));

            Color color = texture->get_pixel(tx, ty);
            if (opacity < 1.0f) {
                color.a *= opacity;
            }
            
            // If opaque, we already wrote the depth, just set the pixel.
            // If semi-transparent, we blend.
            if (color.a >= 1.0f) {
                surface.set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
            } else if (color.a > 0.0f) {
                surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
            }
        }

        u_w += step_u_w;
        v_w += step_v_w;
        one_w += step_one_w;
    }
}

} // namespace

void PerspectiveRasterizer::draw_triangle(
    SurfaceRGBA& surface,
    const Vertex3D& v0_in,
    const Vertex3D& v1_in,
    const Vertex3D& v2_in,
    const SurfaceRGBA* texture,
    float opacity) {

    if (!texture) return;

    const Vertex3D* pts[3] = {&v0_in, &v1_in, &v2_in};
    std::sort(pts, pts + 3, [](const auto* a, const auto* b) {
        return a->position.y < b->position.y;
    });

    Edge e01(*pts[0], *pts[1]);
    Edge e12(*pts[1], *pts[2]);
    Edge e02(*pts[0], *pts[2]);

    bool left_side = e01.step_x < e02.step_x;
    if (e01.y_start == e01.y_end) { // Top-flat
        left_side = pts[1]->position.x < pts[0]->position.x;
    }

    // Top half
    for (int y = e01.y_start; y < e01.y_end; ++y) {
        if (left_side) draw_scanline(surface, y, e01, e02, texture, opacity);
        else draw_scanline(surface, y, e02, e01, texture, opacity);
        e01.step();
        e02.step();
    }

    // Bottom half
    for (int y = e12.y_start; y < e12.y_end; ++y) {
        if (left_side) draw_scanline(surface, y, e12, e02, texture, opacity);
        else draw_scanline(surface, y, e02, e12, texture, opacity);
        e12.step();
        e02.step();
    }
}

void PerspectiveRasterizer::draw_quad(SurfaceRGBA& surface, const PerspectiveWarpQuad& quad) {
    // Split into 2 triangles: (0,1,2) and (0,2,3)
    draw_triangle(surface, quad.v0, quad.v1, quad.v2, quad.texture, quad.opacity);
    draw_triangle(surface, quad.v0, quad.v2, quad.v3, quad.texture, quad.opacity);
}

} // namespace tachyon::renderer2d::raster
