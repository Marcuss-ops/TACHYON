#include "tachyon/renderer2d/project_card.h"

#include "tachyon/core/math/quaternion.h"

#include <cmath>

namespace tachyon {
namespace renderer2d {
namespace {

math::Vector3 rotate_local_point(const math::Vector3& point, float rotation_degrees) {
    const math::Quaternion rotation = math::Quaternion::from_euler({0.0f, 0.0f, rotation_degrees});
    return rotation.rotate_vector(point);
}

} // namespace

ProjectedCard3D project_card_to_screen(
    const RenderableCard3D& card,
    const camera::CameraState& camera,
    float viewport_width,
    float viewport_height) {

    ProjectedCard3D projected;
    projected.quad.texture = card.texture;
    projected.quad.opacity = card.opacity;

    const float half_width = card.width * 0.5f;
    const float half_height = card.height * 0.5f;

    const math::Vector3 local_p0{-half_width, -half_height, 0.0f};
    const math::Vector3 local_p1{ half_width, -half_height, 0.0f};
    const math::Vector3 local_p2{ half_width,  half_height, 0.0f};
    const math::Vector3 local_p3{-half_width,  half_height, 0.0f};

    const math::Vector3 world_p0 = card.center_world + rotate_local_point(local_p0, card.rotation_degrees);
    const math::Vector3 world_p1 = card.center_world + rotate_local_point(local_p1, card.rotation_degrees);
    const math::Vector3 world_p2 = card.center_world + rotate_local_point(local_p2, card.rotation_degrees);
    const math::Vector3 world_p3 = card.center_world + rotate_local_point(local_p3, card.rotation_degrees);

    const math::Vector2 screen_p0 = camera.project_point(world_p0, viewport_width, viewport_height);
    const math::Vector2 screen_p1 = camera.project_point(world_p1, viewport_width, viewport_height);
    const math::Vector2 screen_p2 = camera.project_point(world_p2, viewport_width, viewport_height);
    const math::Vector2 screen_p3 = camera.project_point(world_p3, viewport_width, viewport_height);

    const bool all_behind =
        screen_p0.x < 0.0f && screen_p0.y < 0.0f &&
        screen_p1.x < 0.0f && screen_p1.y < 0.0f &&
        screen_p2.x < 0.0f && screen_p2.y < 0.0f &&
        screen_p3.x < 0.0f && screen_p3.y < 0.0f;
    if (all_behind) {
        return projected;
    }

    projected.visible = true;
    projected.quad.p0 = screen_p0;
    projected.quad.p1 = screen_p1;
    projected.quad.p2 = screen_p2;
    projected.quad.p3 = screen_p3;
    projected.depth_sort_key = static_cast<int>(std::lround(card.center_world.z * 1000.0f));
    return projected;
}

} // namespace renderer2d
} // namespace tachyon
