#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/animation_curve.h"

namespace tachyon::animation {

namespace {

template <typename T, typename SpecT>
T sample_kfs_via_curve(const std::optional<T>& static_val, const std::vector<SpecT>& keyframes, double t, T fallback) {
    if (keyframes.empty()) return static_val.value_or(fallback);
    
    AnimationCurve<T> curve;
    for (const auto& kf : keyframes) {
        Keyframe<T> akf;
        akf.time = kf.time;
        akf.value = kf.value;
        akf.out_mode = kf.interpolation;
        akf.in_mode = kf.interpolation; // Assuming symmetrical for simplicity, text animator didn't set it
        akf.easing = kf.easing;
        akf.bezier = kf.bezier;
        curve.add_keyframe(akf);
    }
    curve.sort();
    return curve.evaluate(t);
}

} // namespace

double sample_scalar(
    const std::optional<double>& static_value,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    double time,
    double fallback
) {
    return sample_kfs_via_curve<double, ScalarKeyframeSpec>(static_value, keyframes, time, fallback);
}

math::Vector2 sample_vector2(
    const std::optional<math::Vector2>& static_value,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    double time,
    math::Vector2 fallback
) {
    return sample_kfs_via_curve<math::Vector2, Vector2KeyframeSpec>(static_value, keyframes, time, fallback);
}

ColorSpec sample_color(
    const std::optional<ColorSpec>& static_value,
    const std::vector<ColorKeyframeSpec>& keyframes,
    double time,
    ColorSpec fallback
) {
    return sample_kfs_via_curve<ColorSpec, ColorKeyframeSpec>(static_value, keyframes, time, fallback);
}

} // namespace tachyon::animation
