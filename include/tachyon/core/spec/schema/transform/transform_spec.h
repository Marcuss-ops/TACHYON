#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/shapes/shape_path.h"

namespace tachyon {

struct Transform2D {
    std::optional<double> position_x;
    std::optional<double> position_y;
    std::optional<double> rotation;
    std::optional<double> scale_x;
    std::optional<double> scale_y;
    AnimatedVector2Spec position_property;
    AnimatedVector2Spec anchor_point;
    AnimatedScalarSpec rotation_property;
    AnimatedVector2Spec scale_property;

    // Motion Path support
    bool motion_path_enabled{false};                           ///< Enable motion path following
    std::optional<shapes::ShapePathSpec> motion_path_shape;    ///< Inline motion path shape (optional)
    std::optional<std::string> motion_path_layer_id;            ///< Reference to another layer's shape
    bool orient_to_path{false};                               ///< Orient layer to path tangent
    AnimatedScalarSpec motion_path_offset_property;             ///< Progress along path (0.0 to 1.0)
};

struct Transform3D {
    AnimatedVector3Spec position_property;
    AnimatedVector3Spec orientation_property; // AE-style orientation (degrees)
    AnimatedVector3Spec rotation_property;    // Euler angles (degrees)
    AnimatedVector3Spec scale_property;
    AnimatedVector3Spec anchor_point_property;
};

} // namespace tachyon
