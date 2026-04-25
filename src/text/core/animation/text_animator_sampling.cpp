#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/renderer2d/animation/easing.h"
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

double sample_scalar_kfs(
    const std::optional<double>& static_val,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    float t) {
    if (keyframes.empty()) return static_val.value_or(0.0);
    if (keyframes.size() == 1U || t <= static_cast<float>(keyframes.front().time)) return keyframes.front().value;
    if (t >= static_cast<float>(keyframes.back().time)) return keyframes.back().value;
    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const double t0 = keyframes[i].time, t1 = keyframes[i+1].time;
        if (static_cast<double>(t) >= t0 && static_cast<double>(t) <= t1) {
            const double raw = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            const double eased = tachyon::animation::apply_easing(raw, keyframes[i].easing, keyframes[i].bezier);
            return keyframes[i].value + eased * (keyframes[i+1].value - keyframes[i].value);
        }
    }
    return keyframes.back().value;
}

::tachyon::ColorSpec sample_color_kfs(
    const std::optional<::tachyon::ColorSpec>& static_val,
    const std::vector<ColorKeyframeSpec>& keyframes,
    float t) {
    if (keyframes.empty()) {
        return static_val.value_or(::tachyon::ColorSpec{255, 255, 255, 255});
    }
    if (keyframes.size() == 1U || t <= static_cast<float>(keyframes.front().time)) {
        return keyframes.front().value;
    }
    if (t >= static_cast<float>(keyframes.back().time)) {
        return keyframes.back().value;
    }

    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const double t0 = keyframes[i].time;
        const double t1 = keyframes[i + 1].time;
        if (static_cast<double>(t) >= t0 && static_cast<double>(t) <= t1) {
            const double raw = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            const float eased = static_cast<float>(tachyon::animation::apply_easing(raw, keyframes[i].easing, keyframes[i].bezier));
            return blend_color(keyframes[i].value, keyframes[i + 1].value, eased);
        }
    }

    return keyframes.back().value;
}

math::Vector2 sample_vector2_kfs(
    const std::optional<math::Vector2>& static_val,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    float t) {
    if (keyframes.empty()) return static_val.value_or(math::Vector2{0.0f, 0.0f});
    if (keyframes.size() == 1U || t <= static_cast<float>(keyframes.front().time)) return keyframes.front().value;
    if (t >= static_cast<float>(keyframes.back().time)) return keyframes.back().value;
    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const double t0 = keyframes[i].time, t1 = keyframes[i+1].time;
        if (static_cast<double>(t) >= t0 && static_cast<double>(t) <= t1) {
            const double raw = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            const float a = static_cast<float>(tachyon::animation::apply_easing(raw, keyframes[i].easing, keyframes[i].bezier));
            const math::Vector2& va = keyframes[i].value;
            const math::Vector2& vb = keyframes[i+1].value;
            return math::Vector2{va.x + (vb.x - va.x) * a, va.y + (vb.y - va.y) * a};
        }
    }
    return keyframes.back().value;
}

} // namespace tachyon::text
