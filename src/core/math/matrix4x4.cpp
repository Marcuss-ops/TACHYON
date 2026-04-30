#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform3.h"

#include <cmath>
#include <limits>

namespace tachyon {
namespace math {
namespace {

constexpr float kEpsilon = 1e-6f;

float determinant3x3(float m00, float m01, float m02,
                    float m10, float m11, float m12,
                    float m20, float m21, float m22) {
    return m00 * (m11 * m22 - m12 * m21)
         - m01 * (m10 * m22 - m12 * m20)
         + m02 * (m10 * m21 - m11 * m20);
}

} // namespace

Matrix4x4 Matrix4x4::identity() {
    Matrix4x4 m;
    m.data[0] = 1.0f;
    m.data[5] = 1.0f;
    m.data[10] = 1.0f;
    m.data[15] = 1.0f;
    return m;
}

Matrix4x4 Matrix4x4::translation(const Vector3& v) {
    Matrix4x4 m = identity();
    m.data[12] = v.x;
    m.data[13] = v.y;
    m.data[14] = v.z;
    return m;
}

Matrix4x4 Matrix4x4::scaling(const Vector3& s) {
    Matrix4x4 m = identity();
    m.data[0] = s.x;
    m.data[5] = s.y;
    m.data[10] = s.z;
    return m;
}

Matrix4x4 Matrix4x4::rotation_x(float angle) {
    Matrix4x4 m = identity();
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    m.data[5] = c;
    m.data[6] = s;
    m.data[9] = -s;
    m.data[10] = c;
    return m;
}

Matrix4x4 Matrix4x4::rotation_y(float angle) {
    Matrix4x4 m = identity();
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    m.data[0] = c;
    m.data[2] = -s;
    m.data[8] = s;
    m.data[10] = c;
    return m;
}

Matrix4x4 Matrix4x4::rotation_z(float angle) {
    Matrix4x4 m = identity();
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    m.data[0] = c;
    m.data[1] = s;
    m.data[4] = -s;
    m.data[5] = c;
    return m;
}

Matrix4x4 Matrix4x4::from_quaternion(const Quaternion& q) {
    const Quaternion n = q.normalized();

    Matrix4x4 m = identity();
    const float xx = n.x * n.x;
    const float yy = n.y * n.y;
    const float zz = n.z * n.z;
    const float xy = n.x * n.y;
    const float xz = n.x * n.z;
    const float yz = n.y * n.z;
    const float wx = n.w * n.x;
    const float wy = n.w * n.y;
    const float wz = n.w * n.z;

    m.data[0] = 1.0f - 2.0f * (yy + zz);
    m.data[1] = 2.0f * (xy + wz);
    m.data[2] = 2.0f * (xz - wy);

    m.data[4] = 2.0f * (xy - wz);
    m.data[5] = 1.0f - 2.0f * (xx + zz);
    m.data[6] = 2.0f * (yz + wx);

    m.data[8] = 2.0f * (xz + wy);
    m.data[9] = 2.0f * (yz - wx);
    m.data[10] = 1.0f - 2.0f * (xx + yy);

    return m;
}

Matrix4x4 Matrix4x4::look_at(const Vector3& eye, const Vector3& target, const Vector3& up) {
    const Vector3 forward = (target - eye).normalized();
    Vector3 right = Vector3::cross(forward, up).normalized();
    if (right.length_squared() <= kEpsilon) {
        right = Vector3::cross(forward, Vector3{0.0f, 0.0f, 1.0f}).normalized();
        if (right.length_squared() <= kEpsilon) {
            right = Vector3{1.0f, 0.0f, 0.0f};
        }
    }
    const Vector3 true_up = Vector3::cross(right, forward).normalized();

    Matrix4x4 m = identity();
    m.data[0] = right.x;
    m.data[4] = right.y;
    m.data[8] = right.z;
    m.data[1] = true_up.x;
    m.data[5] = true_up.y;
    m.data[9] = true_up.z;
    m.data[2] = -forward.x;
    m.data[6] = -forward.y;
    m.data[10] = -forward.z;
    m.data[12] = -Vector3::dot(right, eye);
    m.data[13] = -Vector3::dot(true_up, eye);
    m.data[14] = Vector3::dot(forward, eye);
    return m;
}

Matrix4x4 Matrix4x4::perspective(float fov_y, float aspect, float near_z, float far_z) {
    Matrix4x4 m;
    const float tan_half_fov = std::tan(fov_y * 0.5f);
    m.data[0] = 1.0f / (aspect * tan_half_fov);
    m.data[5] = 1.0f / tan_half_fov;
    m.data[10] = -(far_z + near_z) / (far_z - near_z);
    m.data[11] = -1.0f;
    m.data[14] = -(2.0f * far_z * near_z) / (far_z - near_z);
    return m;
}

Matrix4x4 Matrix4x4::orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    Matrix4x4 m = identity();
    m.data[0] = 2.0f / (right - left);
    m.data[5] = 2.0f / (top - bottom);
    m.data[10] = -2.0f / (far_z - near_z);
    m.data[12] = -(right + left) / (right - left);
    m.data[13] = -(top + bottom) / (top - bottom);
    m.data[14] = -(far_z + near_z) / (far_z - near_z);
    return m;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 res;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += data[row + k * 4] * other.data[k + col * 4];
            }
            res.data[row + col * 4] = sum;
        }
    }
    return res;
}

Matrix4x4 Matrix4x4::inverse_affine() const {
    const float m00 = data[0], m01 = data[4], m02 = data[8];
    const float m10 = data[1], m11 = data[5], m12 = data[9];
    const float m20 = data[2], m21 = data[6], m22 = data[10];

    const float det = determinant3x3(m00, m01, m02,
                                     m10, m11, m12,
                                     m20, m21, m22);
    if (std::abs(det) <= kEpsilon) {
        return identity();
    }

    const float inv_det = 1.0f / det;
    Matrix4x4 inv = identity();

    inv.data[0] =  (m11 * m22 - m12 * m21) * inv_det;
    inv.data[4] = -(m01 * m22 - m02 * m21) * inv_det;
    inv.data[8] =  (m01 * m12 - m02 * m11) * inv_det;

    inv.data[1] = -(m10 * m22 - m12 * m20) * inv_det;
    inv.data[5] =  (m00 * m22 - m02 * m20) * inv_det;
    inv.data[9] = -(m00 * m12 - m02 * m10) * inv_det;

    inv.data[2] =  (m10 * m21 - m11 * m20) * inv_det;
    inv.data[6] = -(m00 * m21 - m01 * m20) * inv_det;
    inv.data[10] = (m00 * m11 - m01 * m10) * inv_det;

    const Vector3 t{data[12], data[13], data[14]};
    const Vector3 inv_t = inv.transform_vector(t * -1.0f);
    inv.data[12] = inv_t.x;
    inv.data[13] = inv_t.y;
    inv.data[14] = inv_t.z;
    return inv;
}

Vector3 Matrix4x4::transform_point(const Vector3& p) const {
    const float x = data[0] * p.x + data[4] * p.y + data[8] * p.z + data[12];
    const float y = data[1] * p.x + data[5] * p.y + data[9] * p.z + data[13];
    const float z = data[2] * p.x + data[6] * p.y + data[10] * p.z + data[14];
    const float w = data[3] * p.x + data[7] * p.y + data[11] * p.z + data[15];

    if (std::abs(w) > kEpsilon && std::abs(w - 1.0f) > kEpsilon) {
        return {x / w, y / w, z / w};
    }
    return {x, y, z};
}

Vector3 Matrix4x4::transform_vector(const Vector3& v) const {
    return {
        data[0] * v.x + data[4] * v.y + data[8] * v.z,
        data[1] * v.x + data[5] * v.y + data[9] * v.z,
        data[2] * v.x + data[6] * v.y + data[10] * v.z
    };
}

Transform3 Matrix4x4::to_transform() const {
    Transform3 trs;
    trs.position = {data[12], data[13], data[14]};

    // Extract scale
    trs.scale.x = Vector3(data[0], data[1], data[2]).length();
    trs.scale.y = Vector3(data[4], data[5], data[6]).length();
    trs.scale.z = Vector3(data[8], data[9], data[10]).length();

    // Remove scale to get rotation matrix
    Matrix4x4 rot = *this;
    if (std::abs(trs.scale.x) > kEpsilon) { rot.data[0] /= trs.scale.x; rot.data[1] /= trs.scale.x; rot.data[2] /= trs.scale.x; }
    if (std::abs(trs.scale.y) > kEpsilon) { rot.data[4] /= trs.scale.y; rot.data[5] /= trs.scale.y; rot.data[6] /= trs.scale.y; }
    if (std::abs(trs.scale.z) > kEpsilon) { rot.data[8] /= trs.scale.z; rot.data[9] /= trs.scale.z; rot.data[10] /= trs.scale.z; }

    // Convert rotation matrix to quaternion
    // Simplified conversion, similar to Quaternion::look_at logic
    float tr = rot.data[0] + rot.data[5] + rot.data[10];
    if (tr > 0.0f) {
        float s = std::sqrt(tr + 1.0f) * 2.0f;
        trs.rotation.w = 0.25f * s;
        trs.rotation.x = (rot.data[6] - rot.data[9]) / s;
        trs.rotation.y = (rot.data[8] - rot.data[2]) / s;
        trs.rotation.z = (rot.data[1] - rot.data[4]) / s;
    } else if ((rot.data[0] > rot.data[5]) && (rot.data[0] > rot.data[10])) {
        float s = std::sqrt(1.0f + rot.data[0] - rot.data[5] - rot.data[10]) * 2.0f;
        trs.rotation.w = (rot.data[6] - rot.data[9]) / s;
        trs.rotation.x = 0.25f * s;
        trs.rotation.y = (rot.data[1] + rot.data[4]) / s;
        trs.rotation.z = (rot.data[8] + rot.data[2]) / s;
    } else if (rot.data[5] > rot.data[10]) {
        float s = std::sqrt(1.0f + rot.data[5] - rot.data[0] - rot.data[10]) * 2.0f;
        trs.rotation.w = (rot.data[8] - rot.data[2]) / s;
        trs.rotation.x = (rot.data[1] + rot.data[4]) / s;
        trs.rotation.y = 0.25f * s;
        trs.rotation.z = (rot.data[6] + rot.data[9]) / s;
    } else {
        float s = std::sqrt(1.0f + rot.data[10] - rot.data[0] - rot.data[5]) * 2.0f;
        trs.rotation.w = (rot.data[1] - rot.data[4]) / s;
        trs.rotation.x = (rot.data[8] + rot.data[2]) / s;
        trs.rotation.y = (rot.data[6] + rot.data[9]) / s;
        trs.rotation.z = 0.25f * s;
    }
    trs.rotation = trs.rotation.normalized();

    return trs;
}

Matrix4x4 compose_trs(const Vector3& translation, const Quaternion& rotation, const Vector3& scale) {
    return Matrix4x4::translation(translation) * (Matrix4x4::from_quaternion(rotation) * Matrix4x4::scaling(scale));
}

Vector3 transform_point(const Matrix4x4& matrix, const Vector3& point) {
    return matrix.transform_point(point);
}

Vector3 transform_vector(const Matrix4x4& matrix, const Vector3& vector) {
    return matrix.transform_vector(vector);
}

Matrix4x4 look_at(const Vector3& eye, const Vector3& target, const Vector3& up) {
    return Matrix4x4::look_at(eye, target, up);
}

Matrix4x4 inverse_affine(const Matrix4x4& matrix) {
    return matrix.inverse_affine();
}

std::ostream& operator<<(std::ostream& out, const Matrix4x4& m) {
    for (int i = 0; i < 4; ++i) {
        out << "[ " << m.data[i] << " " << m.data[i + 4] << " " << m.data[i + 8] << " " << m.data[i + 12] << " ]\n";
    }
    return out;
}

} // namespace math
} // namespace tachyon
