#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"

#include <cmath>

namespace tachyon {
namespace math {

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
    float s = std::sin(angle);
    float c = std::cos(angle);
    m.data[5] = c;
    m.data[6] = s;
    m.data[9] = -s;
    m.data[10] = c;
    return m;
}

Matrix4x4 Matrix4x4::rotation_y(float angle) {
    Matrix4x4 m = identity();
    float s = std::sin(angle);
    float c = std::cos(angle);
    m.data[0] = c;
    m.data[2] = -s;
    m.data[8] = s;
    m.data[10] = c;
    return m;
}

Matrix4x4 Matrix4x4::rotation_z(float angle) {
    Matrix4x4 m = identity();
    float s = std::sin(angle);
    float c = std::cos(angle);
    m.data[0] = c;
    m.data[1] = s;
    m.data[4] = -s;
    m.data[5] = c;
    return m;
}

Matrix4x4 Matrix4x4::from_quaternion(const Quaternion& q) {
    Matrix4x4 m = identity();
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

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
    Vector3 f = (target - eye).normalized();
    Vector3 s = Vector3::cross(f, up).normalized();
    Vector3 u = Vector3::cross(s, f);

    Matrix4x4 m = identity();
    m.data[0] = s.x;
    m.data[4] = s.y;
    m.data[8] = s.z;
    m.data[1] = u.x;
    m.data[5] = u.y;
    m.data[9] = u.z;
    m.data[2] = -f.x;
    m.data[6] = -f.y;
    m.data[10] = -f.z;
    m.data[12] = -Vector3::dot(s, eye);
    m.data[13] = -Vector3::dot(u, eye);
    m.data[14] = Vector3::dot(f, eye);
    return m;
}

Matrix4x4 Matrix4x4::perspective(float fov_y, float aspect, float near_z, float far_z) {
    Matrix4x4 m;
    float tan_half_fov = std::tan(fov_y / 2.0f);
    m.data[0] = 1.0f / (aspect * tan_half_fov);
    m.data[5] = 1.0f / tan_half_fov;
    m.data[10] = -(far_z + near_z) / (far_z - near_z);
    m.data[11] = -1.0f;
    m.data[14] = -(2.0f * far_z * near_z) / (far_z - near_z);
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
    // For affine matrices (Last row is 0,0,0,1)
    // [ R  T ]^-1 = [ R^T  -R^T * T ]
    // Assuming R is orthonormal (no non-uniform scale)
    // Actually, let's do a more robust version if scale is present.
    // For now, implement the orthonormal version (rotation + translation).
    
    Matrix4x4 res = identity();
    // Transpose the 3x3 rotation part
    res.data[0] = data[0]; res.data[4] = data[1]; res.data[8] = data[2];
    res.data[1] = data[4]; res.data[5] = data[5]; res.data[9] = data[6];
    res.data[2] = data[8]; res.data[6] = data[9]; res.data[10] = data[10];

    Vector3 translation(data[12], data[13], data[14]);
    Vector3 inverted_translation;
    inverted_translation.x = -(res.data[0] * translation.x + res.data[4] * translation.y + res.data[8] * translation.z);
    inverted_translation.y = -(res.data[1] * translation.x + res.data[5] * translation.y + res.data[9] * translation.z);
    inverted_translation.z = -(res.data[2] * translation.x + res.data[6] * translation.y + res.data[10] * translation.z);

    res.data[12] = inverted_translation.x;
    res.data[13] = inverted_translation.y;
    res.data[14] = inverted_translation.z;

    return res;
}

Vector3 Matrix4x4::transform_point(const Vector3& p) const {
    return {
        data[0] * p.x + data[4] * p.y + data[8] * p.z + data[12],
        data[1] * p.x + data[5] * p.y + data[9] * p.z + data[13],
        data[2] * p.x + data[6] * p.y + data[10] * p.z + data[14]
    };
}

Vector3 Matrix4x4::transform_vector(const Vector3& v) const {
    return {
        data[0] * v.x + data[4] * v.y + data[8] * v.z,
        data[1] * v.x + data[5] * v.y + data[9] * v.z,
        data[2] * v.x + data[6] * v.y + data[10] * v.z
    };
}

std::ostream& operator<<(std::ostream& out, const Matrix4x4& m) {
    for (int i = 0; i < 4; ++i) {
        out << "[ " << m.data[i] << " " << m.data[i + 4] << " " << m.data[i + 8] << " " << m.data[i + 12] << " ]\n";
    }
    return out;
}

} // namespace math
} // namespace tachyon
