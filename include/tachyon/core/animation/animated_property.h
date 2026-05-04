#pragma once

#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/keyframe.h"
#include <optional>
#include <string>
#include <vector>

namespace tachyon::animation {

/**
 * @brief Configuration for spring-based motion.
 */
struct SpringSpec {
    double stiffness{200.0};
    double damping{20.0};
    double mass{1.0};
    double velocity{0.0};
};

/**
 * @brief Timing and easing configuration for a single keyframe interval.
 */
struct KeyframeTimingSpec {
    EasingPreset easing{EasingPreset::None};
    InterpolationMode interpolation{InterpolationMode::Linear};
    CubicBezierEasing bezier{CubicBezierEasing::linear()};

    SpringSpec spring;

    // After Effects style speed/influence parameters for Bezier handles
    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

/**
 * @brief A generic keyframe with a typed value and timing configuration.
 */
template <typename T>
struct KeyframeSpec {
    double time{0.0};
    T value{};
    KeyframeTimingSpec timing{};
};

/**
 * @brief A keyframe with additional spatial tangents (e.g. for Vector2/3 motion paths).
 */
template <typename T>
struct SpatialKeyframeSpec : KeyframeSpec<T> {
    T tangent_in{};
    T tangent_out{};
};

/**
 * @brief A generic animated property that can have a static value, keyframes, or an expression.
 */
template <typename T>
struct AnimatedProperty {
    std::optional<T> value;
    std::vector<KeyframeSpec<T>> keyframes;
    std::optional<std::string> expression;

    AnimatedProperty() = default;
    explicit AnimatedProperty(const T& v) : value(v) {}

    /**
     * @brief Check if the property has any data or animation.
     */
    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value()
            && keyframes.empty()
            && !expression.has_value();
    }
};

} // namespace tachyon::animation
