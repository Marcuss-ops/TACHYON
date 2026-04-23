#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/matrix4x4.h"

#include <cmath>
#include <algorithm>

namespace tachyon {
namespace math {

Quaternion Quaternion::look_at(const Vector3& eye, const Vector3& target, const Vector3& up) {
    const Vector3 forward = (target - eye).normalized();
    Vector3 right = Vector3::cross(forward, up).normalized();
    if (right.length_squared() < 1e-6f) {
        right = Vector3::cross(forward, Vector3{0, 0, 1}).normalized();
        if (right.length_squared() < 1e-6f) {
            right = Vector3{1, 0, 0};
        }
    }
    const Vector3 true_up = Vector3::cross(right, forward).normalized();

    // Convert rotation matrix [right, true_up, -forward] to quaternion
    // Note: forward is -Z axis in camera space
    const float m00 = right.x;
    const float m10 = right.y;
    const float m20 = right.z;
    const float m01 = true_up.x;
    const float m11 = true_up.y;
    const float m21 = true_up.z;
    const float m02 = -forward.x;
    const float m12 = -forward.y;
    const float m22 = -forward.z;

    const float tr = m00 + m11 + m22;

    if (tr > 0.0f) {
        const float s = std::sqrt(tr + 1.0f) * 2.0f;
        return {
            (m21 - m12) / s,
            (m02 - m20) / s,
            (m10 - m01) / s,
            0.25f * s
        };
    } else if ((m00 > m11) && (m00 > m22)) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        return {
            0.25f * s,
            (m10 + m01) / s,
            (m02 + m20) / s,
            (m21 - m12) / s
        };
    } else if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        return {
            (m10 + m01) / s,
            0.25f * s,
            (m21 + m12) / s,
            (m02 - m20) / s
        };
    } else {
        const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        return {
            (m02 + m20) / s,
            (m21 + m12) / s,
            0.25f * s,
            (m10 - m01) / s
        };
    }
}

Matrix4x4 Quaternion::to_matrix() const {
    return Matrix4x4::from_quaternion(*this);
}

Quaternion Quaternion::slerp(const Quaternion& a, const Quaternion& b, float t) {
    float dot = Quaternion::dot(a, b);
    
    Quaternion q1 = a;
    if (dot < 0.0f) {
        dot = -dot;
        q1 = Quaternion(-a.x, -a.y, -a.z, -a.w);
    }

    if (dot > 0.9995f) {
        return (q1 * (1.0f - t) + b * t).normalized();
    }

    float theta_0 = std::acos(dot);
    float theta = theta_0 * t;
    float sin_theta = std::sin(theta);
    float sin_theta_0 = std::sin(theta_0);

    float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    return (q1 * s0 + b * s1).normalized();
}

} // namespace math
} // namespace tachyon
