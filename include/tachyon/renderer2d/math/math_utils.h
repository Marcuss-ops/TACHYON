#pragma once

#include <cmath>
#include <cstdint>

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"

namespace tachyon::renderer2d::math_utils {

constexpr double kPi = 3.141592653589793238462643383279502884;

double degrees_to_radians(double degrees);

math::Vector2 lerp(const math::Vector2& a, const math::Vector2& b, float t);

math::Vector3 lerp(const math::Vector3& a, const math::Vector3& b, float t);

math::Vector2 sample_bezier_spatial(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t);

math::Vector3 sample_bezier_spatial(const math::Vector3& p0, const math::Vector3& p1, const math::Vector3& p2, const math::Vector3& p3, float t);

double noise(float x, std::uint64_t seed);
double noise(float x, float y, std::uint64_t seed);

} // namespace tachyon::renderer2d::math_utils