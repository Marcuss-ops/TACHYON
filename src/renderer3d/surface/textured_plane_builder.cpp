#include "tachyon/renderer3d/surface/textured_plane_builder.h"

namespace tachyon {
namespace renderer3d {

Mesh3D TexturedPlaneBuilder::build(
    const LayerSurface& surface,
    const math::Vector2& size,
    const math::Vector2& anchor
) {
    Mesh3D mesh;

    float w = size.x;
    float h = size.y;

    float ax = anchor.x;
    float ay = anchor.y;

    float left   = -ax;
    float right  = w - ax;
    float top    = -ay;
    float bottom = h - ay;

    // Standard plane on XY plane facing +Z
    mesh.vertices = {
        {{left,  top,    0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{right, top,    0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{right, bottom, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{left,  bottom, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    };

    mesh.indices = {0, 1, 2, 0, 2, 3};
    mesh.texture = surface.texture;

    return mesh;
}

} // namespace renderer3d
} // namespace tachyon
