#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/matrix3x3.h"

namespace tachyon {
namespace math {

struct Transform2 {
    Vector2 position{Vector2::zero()};
    float rotation_rad{0.0f};
    Vector2 scale{Vector2::one()};
    Vector2 anchor_point{Vector2::zero()};

    static Transform2 identity() { return {}; }

    [[nodiscard]] Matrix3x3 to_matrix() const {
        const Matrix3x3 t = Matrix3x3::make_translation(position);
        const Matrix3x3 r = Matrix3x3::make_rotation(rotation_rad);
        const Matrix3x3 s = Matrix3x3::make_scale(scale.x, scale.y);
        const Matrix3x3 a = Matrix3x3::make_translation({-anchor_point.x, -anchor_point.y});
        return t * (r * (s * a));
    }

    [[nodiscard]] Matrix3x3 to_inverse_matrix() const {
        return to_matrix().inverse();
    }
};

} // namespace math
} // namespace tachyon
