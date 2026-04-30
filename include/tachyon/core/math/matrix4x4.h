#pragma once

#include "tachyon/core/math/vector3.h"

#include <array>
#include <ostream>

namespace tachyon {
namespace math {

struct Quaternion;

/**
 * 4x4 matrix, column-major storage.
 * Identity:
 * [ 0  4  8 12 ]
 * [ 1  5  9 13 ]
 * [ 2  6 10 14 ]
 * [ 3  7 11 15 ]
 */
struct Matrix4x4 {
    std::array<float, 16> data{};

    Matrix4x4() = default;
    explicit Matrix4x4(const float* src) { std::copy(src, src + 16, data.begin()); }

    static Matrix4x4 identity();
    static Matrix4x4 translation(const Vector3& velocity);
    static Matrix4x4 scaling(const Vector3& scale);
    static Matrix4x4 rotation_x(float angle_rad);
    static Matrix4x4 rotation_y(float angle_rad);
    static Matrix4x4 rotation_z(float angle_rad);

    static Matrix4x4 from_quaternion(const Quaternion& q);

    static Matrix4x4 look_at(const Vector3& eye, const Vector3& target, const Vector3& up);

    static Matrix4x4 perspective(float fov_y_rad, float aspect, float near_z, float far_z);
    static Matrix4x4 orthographic(float left, float right, float bottom, float top, float near_z, float far_z);

    float& operator[](std::size_t index) { return data[index]; }
    const float& operator[](std::size_t index) const { return data[index]; }

    Matrix4x4 operator*(const Matrix4x4& other) const;

    [[nodiscard]] Matrix4x4 inverse_affine() const;

    [[nodiscard]] Vector3 transform_point(const Vector3& v) const;
    [[nodiscard]] Vector3 transform_vector(const Vector3& v) const;
    [[nodiscard]] struct Transform3 to_transform() const;
};

[[nodiscard]] Matrix4x4 compose_trs(const Vector3& translation, const Quaternion& rotation, const Vector3& scale);
[[nodiscard]] Vector3 transform_point(const Matrix4x4& matrix, const Vector3& point);
[[nodiscard]] Vector3 transform_vector(const Matrix4x4& matrix, const Vector3& vector);
[[nodiscard]] Matrix4x4 look_at(const Vector3& eye, const Vector3& target, const Vector3& up);
[[nodiscard]] Matrix4x4 inverse_affine(const Matrix4x4& matrix);

std::ostream& operator<<(std::ostream& out, const Matrix4x4& m);

} // namespace math
} // namespace tachyon
