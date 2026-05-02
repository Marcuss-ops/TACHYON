#pragma once

#include "tachyon/renderer3d/core/mesh_types.h"
#include "tachyon/renderer3d/surface/layer_surface.h"
#include "tachyon/core/math/vector2.h"

namespace tachyon {
namespace renderer3d {

class TexturedPlaneBuilder {
public:
    static Mesh3D build(
        const LayerSurface& surface,
        const math::Vector2& size,
        const math::Vector2& anchor
    );
};

} // namespace renderer3d
} // namespace tachyon
