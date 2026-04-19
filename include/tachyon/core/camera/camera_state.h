#pragma once

#include "tachyon/core/math/transform3.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/vector2.h"

namespace tachyon {
namespace camera {

/**
 * Encapsulates the state of a camera, including its physical/optical properties
 * and its position in the world.
 */
struct CameraState {
    math::Transform3 transform;
    
    float fov_y_rad{0.785398f}; // ~45 degrees
    float aspect{1.777778f};   // 16:9
    float near_z{0.1f};
    float far_z{1000.0f};

    CameraState() = default;

    /**
     * View Matrix: Inverse of the camera's world transform.
     */
    [[nodiscard]] math::Matrix4x4 get_view_matrix() const {
        return transform.to_inverse_matrix();
    }

    /**
     * Projection Matrix: Maps camera-space to clip-space.
     */
    [[nodiscard]] math::Matrix4x4 get_projection_matrix() const {
        return math::Matrix4x4::perspective(fov_y_rad, aspect, near_z, far_z);
    }

    /**
     * Full MVP Matrix for a given model transform.
     */
    [[nodiscard]] math::Matrix4x4 get_mvp_matrix(const math::Matrix4x4& model) const {
        return get_projection_matrix() * (get_view_matrix() * model);
    }

    /**
     * Projects a world-space point to screen coordinates.
     * Returns a Vector2 where (0,0) is top-left and (w, h) is bottom-right.
     * Returns { -1, -1 } if the point is behind the camera.
     */
    [[nodiscard]] math::Vector2 project_point(const math::Vector3& world_p, float viewport_w, float viewport_h) const;
};

} // namespace camera
} // namespace tachyon
