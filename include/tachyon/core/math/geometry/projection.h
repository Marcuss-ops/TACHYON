#pragma once

#include "tachyon/core/math/algebra/matrix4x4.h"

namespace tachyon {
namespace math {

struct Projection {
    enum class Type { Perspective, Orthographic };

    Type type{Type::Perspective};
    float fov_y_rad{0.785398f};
    float aspect{1.777778f};
    float near_z{0.1f};
    float far_z{1000.0f};
    float left{-1.0f};
    float right{1.0f};
    float bottom{-1.0f};
    float top{1.0f};

    [[nodiscard]] Matrix4x4 to_matrix() const {
        if (type == Type::Orthographic) {
            return Matrix4x4::orthographic(left, right, bottom, top, near_z, far_z);
        }
        return Matrix4x4::perspective(fov_y_rad, aspect, near_z, far_z);
    }
};

} // namespace math
} // namespace tachyon

