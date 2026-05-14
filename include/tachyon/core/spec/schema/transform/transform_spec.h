#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/math/transform2.h"
#include <optional>

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

    [[nodiscard]] math::Transform2 to_transform2() const {
        math::Transform2 t;
        t.position = {
            static_cast<float>(position_x.value_or(0.0)),
            static_cast<float>(position_y.value_or(0.0))
        };
        t.rotation_rad = static_cast<float>(rotation.value_or(0.0));
        t.scale = {
            static_cast<float>(scale_x.value_or(100.0) / 100.0),
            static_cast<float>(scale_y.value_or(100.0) / 100.0)
        };
        // Note: Anchor point and properties are usually handled by the evaluator/sampler.
        // This provides a base transform for static evaluation.
        return t;
    }
};


} // namespace tachyon
