#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/types/color_spec.h"

namespace tachyon::animation {

/**
 * @brief Canonical linear interpolation for scalars.
 */
double lerp_scalar(double a, double b, double t);

/**
 * @brief Canonical linear interpolation for Vector2.
 */
math::Vector2 lerp_vector2(const math::Vector2& a, const math::Vector2& b, double t);

/**
 * @brief Canonical linear interpolation for Vector3.
 */
math::Vector3 lerp_vector3(const math::Vector3& a, const math::Vector3& b, double t);

/**
 * @brief Canonical linear interpolation for ColorSpec.
 */
ColorSpec lerp_color(const ColorSpec& a, const ColorSpec& b, double t);

} // namespace tachyon::animation
