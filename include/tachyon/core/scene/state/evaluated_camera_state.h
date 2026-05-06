#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/camera/camera_state.h"
#include <string>
#include <optional>

namespace tachyon::scene {

/**
 * @brief Represents the fully evaluated state of a camera for a specific frame.
 * 
 * Includes AE-style optical properties and two-node target tracking.
 */
struct EvaluatedCameraState {
    bool available{false};
    std::string layer_id;
    std::string name{"Default Camera"};

    // Camera Type & Rig
    std::string camera_type{"one_node"}; // "one_node" | "two_node"
    std::string parent_id;

    // Resolved Transform
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 point_of_interest{0.0f, 0.0f, 0.0f};
    math::Vector3 up{0.0f, 1.0f, 0.0f};
    float roll{0.0f};

    // Matrices (Fully Resolved)
    math::Matrix4x4 view_matrix{math::Matrix4x4::identity()};
    math::Matrix4x4 projection_matrix{math::Matrix4x4::identity()};

    // Optics & AE Properties
    float zoom{877.0f};           // AE-style zoom (px)
    float fov_y_rad{0.68f};       // Computed FOV
    float aspect{1.777778f};
    float near_clip{0.1f};
    float far_clip{100000.0f};

    // Depth of Field
    bool  dof_enabled{false};
    float focus_distance{1000.0f};
    float aperture{4.0f};         // f-stop
    float blur_level{100.0f};     // % intensity

    // Legacy/Internal Bridge State
    camera::CameraState camera;

    // Temporal State (for Motion Blur)
    math::Matrix4x4 previous_world_matrix{math::Matrix4x4::identity()};
};

} // namespace tachyon::scene
