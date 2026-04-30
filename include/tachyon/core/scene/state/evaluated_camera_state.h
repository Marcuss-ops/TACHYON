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

    // Camera Type
    std::string camera_type{"one_node"}; // "one_node" | "two_node"
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 point_of_interest{0.0f, 0.0f, 0.0f}; // only for two-node
    std::optional<math::Vector3> previous_position;
    std::optional<math::Vector3> previous_point_of_interest;

    // Optics
    float zoom{877.0f};           // Lens-to-image plane distance (px at 100%)
    float focal_length{50.0f};    // mm
    float film_size{36.0f};       // mm, default 35mm full frame
    float angle_of_view{39.6f};   // degrees, calculated from focal_length/film_size

    // Depth of Field
    bool  dof_enabled{false};
    float focus_distance{1000.0f};
    float aperture{4.0f};         // f-stop
    float blur_level{100.0f};     // % of bokeh intensity

    // Core Camera State (View/Projection matrices)
    camera::CameraState camera;

    // Temporal State (for Motion Blur)
    math::Matrix4x4 previous_camera_matrix{math::Matrix4x4::identity()};
};

} // namespace tachyon::scene
