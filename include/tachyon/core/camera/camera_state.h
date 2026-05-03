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
    
    // Two-Node Camera
    math::Vector3 target_position{0.0f, 0.0f, 0.0f};
    math::Vector3 up{0.0f, -1.0f, 0.0f};
    bool use_target{false};

    // Physical Camera Model
    float focal_length_mm{35.0f};
    float sensor_width_mm{36.0f}; // Full frame
    float aperture_fstop{2.8f};
    float focus_distance{10.0f};

    float fov_y_rad{0.785398f}; // Computed from focal length if not manual
    float aspect{1.777778f};   // 16:9
    float near_z{0.1f};
    float far_z{1000.0f};

    CameraState() = default;

    /**
     * View Matrix: Inverse of the camera's world transform.
     */
    [[nodiscard]] math::Matrix4x4 get_view_matrix() const {
        if (use_target) {
            return math::Matrix4x4::look_at(transform.position, target_position, up);
        }
        return transform.to_inverse_matrix();
    }

    /**
     * Projection Matrix: Maps camera-space to clip-space.
     */
    [[nodiscard]] math::Matrix4x4 get_projection_matrix() const {
        // Calculate FOV from focal length if needed
        float effective_fov = fov_y_rad;
        if (focal_length_mm > 0.0f) {
            // fov = 2 * atan(sensor_height / (2 * focal_length))
            float sensor_height = sensor_width_mm / aspect;
            effective_fov = 2.0f * std::atan(sensor_height / (2.0f * focal_length_mm));
        }
        return math::Matrix4x4::perspective(effective_fov, aspect, near_z, far_z);
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
