#pragma once

#include <cmath>
#include <ostream>

namespace tachyon {
namespace math {

struct Vector3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    Vector3() = default;
    constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    static constexpr Vector3 zero() { return {0.0f, 0.0f, 0.0f}; }
    static constexpr Vector3 one() { return {1.0f, 1.0f, 1.0f}; }
    static constexpr Vector3 up() { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Vector3 forward() { return {0.0f, 0.0f, -1.0f}; } // Right-handed, camera looks -Z

    constexpr Vector3 operator-() const { return {-x, -y, -z}; }
    constexpr Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    constexpr Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    constexpr Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    constexpr Vector3 operator*(const Vector3& other) const { return {x * other.x, y * other.y, z * other.z}; }
    constexpr Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vector3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

    [[nodiscard]] float length_squared() const { return x * x + y * y + z * z; }
    [[nodiscard]] float length() const { return std::sqrt(length_squared()); }

    [[nodiscard]] Vector3 normalized() const {
        float len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return zero();
    }

    [[nodiscard]] static float dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    [[nodiscard]] static Vector3 cross(const Vector3& a, const Vector3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

inline Vector3 operator*(float scalar, const Vector3& v) { return v * scalar; }

inline std::ostream& operator<<(std::ostream& out, const Vector3& v) {
    return out << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

} // namespace math
} // namespace tachyon
