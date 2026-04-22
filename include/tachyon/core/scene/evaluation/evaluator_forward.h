#pragma once

// Forward declarations for types needed by evaluator.cpp
namespace tachyon { namespace math {
    struct Vector2;
    struct Vector3;
    struct Quaternion;
    struct Matrix4x4;
    struct Transform2;
} }

namespace tachyon { namespace scene {

class LayerSpec;

namespace {

double degrees_to_radians(double degrees);

template <typename T>
T lerp(const T& a, const T& b, float t);

template <typename T>
T sample_bezier_spatial(const T& p0, const T& p1, const T& p2, const T& p3, float t);

math::Vector2 fallback_position(const LayerSpec& layer);
double fallback_rotation(const LayerSpec& layer);
math::Vector2 fallback_scale(const LayerSpec& layer);

} // namespace
} // namespace scene
} // namespace tachyon