#include "tachyon/core/math/matrix4x4.h"

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

std::ostream& operator<<(std::ostream& out, const Matrix4x4& m) {
    for (int i = 0; i < 4; ++i) {
        out << "[ " << m.data[i] << " " << m.data[i + 4] << " " << m.data[i + 8] << " " << m.data[i + 12] << " ]\n";
    }
    return out;
}

} // namespace math
} // namespace tachyon
