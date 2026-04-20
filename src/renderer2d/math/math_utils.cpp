#include "tachyon/renderer2d/math/math_utils.h"

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

} // namespace tachyon::renderer2d::math_utils