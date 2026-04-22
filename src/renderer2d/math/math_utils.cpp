#include "tachyon/renderer2d/math/math_utils.h"
#include "tachyon/runtime/core/contracts/math_contract.h"

namespace tachyon::renderer2d::math_utils {

double degrees_to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

math::Vector2 lerp(const math::Vector2& a, const math::Vector2& b, float t) {
    return a * (1.0f - t) + b * t;
}

math::Vector3 lerp(const math::Vector3& a, const math::Vector3& b, float t) {
    return a * (1.0f - t) + b * t;
}

math::Vector2 sample_bezier_spatial(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t) {
    const float it = 1.0f - t;
    return p0 * (it * it * it) + p1 * (3.0f * it * it * t) + p2 * (3.0f * it * t * t) + p3 * (t * t * t);
}

math::Vector3 sample_bezier_spatial(const math::Vector3& p0, const math::Vector3& p1, const math::Vector3& p2, const math::Vector3& p3, float t) {
    const float it = 1.0f - t;
    return p0 * (it * it * it) + p1 * (3.0f * it * it * t) + p2 * (3.0f * it * t * t) + p3 * (t * t * t);
}

double noise(float x, std::uint64_t seed) {
    const std::uint64_t i = static_cast<std::uint64_t>(std::floor(x));
    const float f = x - static_cast<float>(i);
    const float u = f * f * (3.0f - 2.0f * f);

    const double v0 = math_contract::noise(i + seed);
    const double v1 = math_contract::noise(i + 1 + seed);

    return math_contract::lerp(v0, v1, u);
}

double noise(float x, float y, std::uint64_t seed) {
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

} // namespace tachyon::renderer2d::math_utils