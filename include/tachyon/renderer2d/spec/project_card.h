#pragma once

#include "tachyon/core/camera/camera_state.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/renderer2d/raster/draw_command.h"

namespace tachyon {
namespace renderer2d {

struct RenderableCard3D {
    TextureHandle texture;
    math::Vector3 center_world{math::Vector3::zero()};
    float width{0.0f};
    float height{0.0f};
    float rotation_degrees{0.0f};
    float opacity{1.0f};
};

struct ProjectedCard3D {
    bool visible{false};
    int depth_sort_key{0};
    TexturedQuadCommand quad;
};

ProjectedCard3D project_card_to_screen(
    const RenderableCard3D& card,
    const camera::CameraState& camera,
    float viewport_width,
    float viewport_height);

} // namespace renderer2d
} // namespace tachyon

