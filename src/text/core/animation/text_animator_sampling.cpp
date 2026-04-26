#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/animation_curve.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

template <typename T>
T lerp_channel(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    return {
        lerp_channel<std::uint8_t>(a.r, b.r, clamped),
        lerp_channel<std::uint8_t>(a.g, b.g, clamped),
        lerp_channel<std::uint8_t>(a.b, b.b, clamped),
        lerp_channel<std::uint8_t>(a.a, b.a, clamped)
    };
}

} // namespace

namespace {
template <typename T, typename SpecT>
T sample_kfs_via_curve(const std::optional<T>& static_val, const std::vector<SpecT>& keyframes, float t, T fallback) {
    if (keyframes.empty()) return static_val.value_or(fallback);
    
    animation::AnimationCurve<T> curve;
    for (const auto& kf : keyframes) {
        animation::Keyframe<T> akf;
        akf.time = kf.time;
        akf.value = kf.value;
        akf.out_mode = kf.interpolation;
        akf.easing = kf.easing;
        akf.bezier = kf.bezier;
        akf.spring = kf.spring;
        curve.add_keyframe(akf);
    }
    curve.sort();
    return curve.evaluate(static_cast<double>(t));
}
}

double sample_scalar_kfs(
    const std::optional<double>& static_val,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    float t) {
    return sample_kfs_via_curve<double, ScalarKeyframeSpec>(static_val, keyframes, t, 0.0);
}

::tachyon::ColorSpec sample_color_kfs(
    const std::optional<::tachyon::ColorSpec>& static_val,
    const std::vector<ColorKeyframeSpec>& keyframes,
    float t) {
    return sample_kfs_via_curve<::tachyon::ColorSpec, ColorKeyframeSpec>(static_val, keyframes, t, ::tachyon::ColorSpec{255, 255, 255, 255});
}

math::Vector2 sample_vector2_kfs(
    const std::optional<math::Vector2>& static_val,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    float t) {
    return sample_kfs_via_curve<math::Vector2, Vector2KeyframeSpec>(static_val, keyframes, t, math::Vector2{0.0f, 0.0f});
}

} // namespace tachyon::text
