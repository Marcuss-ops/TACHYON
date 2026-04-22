#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"

namespace tachyon {

struct Transform2D {
    std::optional<double> position_x;
    std::optional<double> position_y;
    std::optional<double> rotation;
    std::optional<double> scale_x;
    std::optional<double> scale_y;
    AnimatedVector2Spec position_property;
    AnimatedScalarSpec rotation_property;
    AnimatedVector2Spec scale_property;
};

struct Transform3D {
    AnimatedVector3Spec position_property;
    AnimatedVector3Spec orientation_property; // AE-style orientation (degrees)
    AnimatedVector3Spec rotation_property;    // Euler angles (degrees)
    AnimatedVector3Spec scale_property;
    AnimatedVector3Spec anchor_point_property;
};

} // namespace tachyon
