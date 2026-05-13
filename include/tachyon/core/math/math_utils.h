#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/runtime/core/contracts/math_contract.h"
#include <cmath>
#include <cstdint>

namespace tachyon::math {

constexpr double kPi = 3.14159265358979323846;
constexpr float kPiF = 3.14159265359f;

inline double degrees_to_radians(double degrees) {
    return degrees * (kPi / 180.0);
}

inline float degrees_to_radians(float degrees) {
    return degrees * (kPiF / 180.0f);
}

inline Vector2 lerp(const Vector2& a, const Vector2& b, float t) {
    return a * (1.0f - t) + b * t;
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


inline double noise(float x, std::uint64_t seed) {
    const std::uint64_t i = static_cast<std::uint64_t>(std::floor(x));
    const float f = x - static_cast<float>(i);
    const float u = f * f * (3.0f - 2.0f * f);

    const double v0 = math_contract::noise(i + seed);
    const double v1 = math_contract::noise(i + 1 + seed);

    return math_contract::lerp(v0, v1, u);
}

inline double noise(float x, float y, std::uint64_t seed) {
    const std::uint64_t ix = static_cast<std::uint64_t>(std::floor(x));
    const std::uint64_t iy = static_cast<std::uint64_t>(std::floor(y));
    const float fx = x - static_cast<float>(ix);
    const float fy = y - static_cast<float>(iy);
    const float ux = fx * fx * (3.0f - 2.0f * fx);
    const float uy = fy * fy * (3.0f - 2.0f * fy);

    const double v00 = math_contract::noise(math_contract::splitmix64(ix + seed) + iy);
    const double v10 = math_contract::noise(math_contract::splitmix64(ix + 1 + seed) + iy);
    const double v01 = math_contract::noise(math_contract::splitmix64(ix + seed) + iy + 1);
    const double v11 = math_contract::noise(math_contract::splitmix64(ix + 1 + seed) + iy + 1);

    const double res_y0 = math_contract::lerp(v00, v10, ux);
    const double res_y1 = math_contract::lerp(v01, v11, ux);

    return math_contract::lerp(res_y0, res_y1, uy);
}

} // namespace tachyon::math
