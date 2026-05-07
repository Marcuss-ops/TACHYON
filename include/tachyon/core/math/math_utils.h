#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include <cmath>

namespace tachyon::math {

constexpr double kPi = 3.14159265358979323846;
constexpr float kPiF = 3.14159265359f;

inline double degrees_to_radians(double degrees) {
    return degrees * (kPi / 180.0);
}

inline float degrees_to_radians(float degrees) {
    return degrees * (kPiF / 180.0f);
}


/**
 * @brief Performs cubic bezier sampling for spatial paths.
 */
inline Vector2 sample_bezier_spatial(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float t) {
    const float it = 1.0f - t;
    const float it2 = it * it;
    const float it3 = it2 * it;
    const float t2 = t * t;
    const float t3 = t2 * t;
    return p0 * it3 + p1 * (3.0f * it2 * t) + p2 * (3.0f * it * t2) + p3 * t3;
}

inline Vector3 sample_bezier_spatial(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
    const float it = 1.0f - t;
    const float it2 = it * it;
    const float it3 = it2 * it;
    const float t2 = t * t;
    const float t3 = t2 * t;
    return p0 * it3 + p1 * (3.0f * it2 * t) + p2 * (3.0f * it * t2) + p3 * t3;
}

} // namespace tachyon::math
