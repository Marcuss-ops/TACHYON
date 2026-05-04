#include "tachyon/core/animation/property_interpolation.h"
#include <algorithm>
#include <cmath>

namespace tachyon::animation {

double lerp_scalar(double a, double b, double t) {
    return a + (b - a) * t;
}

math::Vector2 lerp_vector2(const math::Vector2& a, const math::Vector2& b, double t) {
    return {
        static_cast<float>(a.x + (b.x - a.x) * t),
        static_cast<float>(a.y + (b.y - a.y) * t)
    };
}

math::Vector3 lerp_vector3(const math::Vector3& a, const math::Vector3& b, double t) {
    return {
        static_cast<float>(a.x + (b.x - a.x) * t),
        static_cast<float>(a.y + (b.y - a.y) * t),
        static_cast<float>(a.z + (b.z - a.z) * t)
    };
}

ColorSpec lerp_color(const ColorSpec& a, const ColorSpec& b, double t) {
    auto l = [t](std::uint8_t v1, std::uint8_t v2) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::clamp(v1 + (v2 - v1) * t + 0.5, 0.0, 255.0));
    };
    return {
        l(a.r, b.r),
        l(a.g, b.g),
        l(a.b, b.b),
        l(a.a, b.a)
    };
}

} // namespace tachyon::animation
