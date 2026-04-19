#pragma once

#include "tachyon/core/math/vector3.h"

#include <cmath>

namespace tachyon {
namespace math {

struct Quaternion {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float w{1.0f};

    Quaternion() = default;
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static Quaternion identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    static Quaternion from_axis_angle(const Vector3& axis, float angle_rad) {
        float half_angle = angle_rad * 0.5f;
        float s = std::sin(half_angle);
        Vector3 aligned_axis = axis.normalized();
        return {
            aligned_axis.x * s,
            aligned_axis.y * s,
            aligned_axis.z * s,
            std::cos(half_angle)
        };
    }

    static Quaternion from_euler(const Vector3& degrees_yxz) {
        float rad_x = degrees_yxz.x * (3.14159265f / 180.0f);
        float rad_y = degrees_yxz.y * (3.14159265f / 180.0f);
        float rad_z = degrees_yxz.z * (3.14159265f / 180.0f);

        Quaternion qx = from_axis_angle({1, 0, 0}, rad_x);
        Quaternion qy = from_axis_angle({0, 1, 0}, rad_y);
        Quaternion qz = from_axis_angle({0, 0, 1}, rad_z);

        // Order Y -> X -> Z (Tait-Bryan)
        return qz * qx * qy;
    }

    Quaternion operator*(const Quaternion& q) const {
        return {
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y + y * q.w + z * q.x - x * q.z,
            w * q.z + z * q.w + x * q.y - y * q.x,
            w * q.w - x * q.x - y * q.y - z * q.z
        };
    }

    [[nodiscard]] float length_squared() const { return x * x + y * y + z * z + w * w; }
    [[nodiscard]] float length() const { return std::sqrt(length_squared()); }

    [[nodiscard]] Quaternion normalized() const {
        float len = length();
        if (len > 0.0f) {
            float inv_len = 1.0f / len;
            return {x * inv_len, y * inv_len, z * inv_len, w * inv_len};
        }
        return identity();
    }

    struct Matrix4x4 to_matrix() const;
};

} // namespace math
} // namespace tachyon
