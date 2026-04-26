#pragma once

#include <cmath>
#include <ostream>
#include <nlohmann/json.hpp>

namespace tachyon {
namespace math {

struct Vector2 {
    float x{0.0f};
    float y{0.0f};

    Vector2() = default;
    constexpr Vector2(float x, float y) : x(x), y(y) {}

    static constexpr Vector2 zero() { return {0.0f, 0.0f}; }
    static constexpr Vector2 one() { return {1.0f, 1.0f}; }

    constexpr Vector2 operator+(const Vector2& other) const { return {x + other.x, y + other.y}; }
    constexpr Vector2 operator-(const Vector2& other) const { return {x - other.x, y - other.y}; }
    constexpr Vector2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vector2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

    [[nodiscard]] float length_squared() const { return x * x + y * y; }
    [[nodiscard]] float length() const { return std::sqrt(length_squared()); }

    [[nodiscard]] Vector2 normalized() const {
        float len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return zero();
    }

    [[nodiscard]] static float dot(const Vector2& a, const Vector2& b) {
        return a.x * b.x + a.y * b.y;
    }
};

inline Vector2 operator*(float scalar, const Vector2& v) { return v * scalar; }

inline std::ostream& operator<<(std::ostream& out, const Vector2& v) {
    return out << "(" << v.x << ", " << v.y << ")";
}

// JSON serialization declarations (implementations in vector2_serialize.cpp)
void to_json(nlohmann::json& j, const Vector2& v);
void from_json(const nlohmann::json& j, Vector2& v);

} // namespace math
} // namespace tachyon
