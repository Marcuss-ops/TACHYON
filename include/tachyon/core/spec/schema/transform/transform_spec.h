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
    AnimatedVector2Spec anchor_point;
    AnimatedScalarSpec rotation_property;
    AnimatedVector2Spec scale_property;
};


} // namespace tachyon
