#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/matrix4x4.h"

namespace tachyon {
namespace math {

struct Transform2 {
    Vector2 position{Vector2::zero()};
    Vector2 anchor_point{Vector2::zero()};
    float rotation_rad{0.0f};
    Vector2 scale{Vector2::one()};

    static Transform2 identity() { return {}; }

    [[nodiscard]] Matrix4x4 to_matrix() const {
        const Matrix4x4 t = Matrix4x4::translation({position.x, position.y, 0.0f});
        const Matrix4x4 r = Matrix4x4::rotation_z(rotation_rad);
        const Matrix4x4 s = Matrix4x4::scaling({scale.x, scale.y, 1.0f});
        const Matrix4x4 ap = Matrix4x4::translation({-anchor_point.x, -anchor_point.y, 0.0f});
        return t * (r * (s * ap));
    }

    [[nodiscard]] Matrix4x4 to_inverse_matrix() const {
        return to_matrix().inverse_affine();
    }
};

} // namespace math
} // namespace tachyon
