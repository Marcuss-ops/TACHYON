#pragma once

#include "tachyon/renderer3d/core/mesh_types.h"
#include "tachyon/core/render/scene_3d.h"
#include "tachyon/renderer2d/resource/render_context.h"

namespace tachyon {
namespace renderer3d {

class I3DModifier {
public:
    virtual ~I3DModifier() = default;

    virtual void apply(
        Mesh3D& mesh,
        const ResolvedModifier3D& resolved,
        const renderer2d::RenderContext& ctx
    ) = 0;
};

} // namespace renderer3d
} // namespace tachyon
