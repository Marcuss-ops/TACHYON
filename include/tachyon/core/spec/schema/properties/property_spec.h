#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/keyframe.h"
#include "tachyon/core/spec/schema/objects/path_spec.h"
#include "tachyon/core/spec/schema/objects/mask_spec.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/template_spec.h"

#include <vector>
#include <optional>
#include <string>
#include <utility>

namespace tachyon {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

struct ScalarKeyframeSpec {
    double time{0.0};
    double value{0.0};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    struct SpringSpec {
        double stiffness{200.0};
        double damping{20.0};
        double mass{1.0};
        double velocity{0.0};
    } spring;
    
    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct Vector2KeyframeSpec {
    double time{0.0};
    math::Vector2 value{math::Vector2::zero()};
    math::Vector2 tangent_in{math::Vector2::zero()};
    math::Vector2 tangent_out{math::Vector2::zero()};

    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    struct SpringSpec {
        double stiffness{200.0};
        double damping{20.0};
        double mass{1.0};
        double velocity{0.0};
    } spring;

    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct ColorKeyframeSpec {
    double time{0.0};
    ColorSpec value{255, 255, 255, 255};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};

    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct WiggleSpec {
    bool enabled{false};
    double frequency{1.0};
    double amplitude{50.0};
    uint64_t seed{0};
    int octaves{1};
};

struct AnimatedScalarSpec {
    std::optional<double> value;
    std::vector<ScalarKeyframeSpec> keyframes;
    PropertyBinding binding;
    std::optional<AudioBandType> audio_band;
    double audio_min{0.0};
    double audio_max{1.0};
    std::optional<std::string> expression;
    WiggleSpec wiggle;

    AnimatedScalarSpec() = default;
    AnimatedScalarSpec(double v) : value(v) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !binding.active && !audio_band.has_value() && !expression.has_value() && !wiggle.enabled;
    }
};

struct AnimatedVector2Spec {
    std::optional<math::Vector2> value;
    std::vector<Vector2KeyframeSpec> keyframes;
    std::optional<std::string> expression;

    AnimatedVector2Spec() = default;
    explicit AnimatedVector2Spec(const math::Vector2& v) : value(v) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !expression.has_value();
    }
};

struct AnimatedVector3Spec {
    std::optional<math::Vector3> value;
    struct Keyframe {
        double time{0.0};
        math::Vector3 value{math::Vector3::zero()};
        math::Vector3 tangent_in{math::Vector3::zero()};
        math::Vector3 tangent_out{math::Vector3::zero()};

        animation::EasingPreset easing{animation::EasingPreset::None};
        animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
        animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};

        double speed_in{0.0};
        double influence_in{33.333333333};
        double speed_out{0.0};
        double influence_out{33.333333333};
    };
    std::vector<Keyframe> keyframes;
    std::optional<std::string> expression;

    AnimatedVector3Spec() = default;
    AnimatedVector3Spec(const math::Vector3& v) : value(v) {}

    void set_value(const math::Vector3& v) { value = v; }
    void add_keyframe(const Keyframe& kf) { keyframes.push_back(kf); }
    void set_expression(std::string expr) { expression = std::move(expr); }

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !expression.has_value();
    }
};

struct AnimatedColorSpec {
    std::optional<ColorSpec> value;
    std::vector<ColorKeyframeSpec> keyframes;
    std::optional<std::string> expression;

    AnimatedColorSpec() = default;
    AnimatedColorSpec(const ColorSpec& v) : value(v) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !expression.has_value();
    }
};

struct MaskPathKeyframeSpec {
    double time{0.0};
    spec::MaskPath value;
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier;
    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct AnimatedMaskPathSpec {
    std::optional<spec::MaskPath> value;
    std::vector<MaskPathKeyframeSpec> keyframes;
    std::optional<std::string> expression;
    WiggleSpec wiggle;

    AnimatedMaskPathSpec() = default;
    AnimatedMaskPathSpec(const spec::MaskPath& v) : value(v) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
};

/**
 * @brief Effect specification with support for animated parameters over time.
 * 
 * Allows effects to have keyframed scalar/color/string parameters instead of static values.
 * Example: Blur radius animating from 0 to 50 over 2 seconds.
 */
struct AnimatedEffectSpec {
    bool enabled{true};
    std::string type;
    
    // Static values (backward compatible with EffectSpec)
    std::map<std::string, double> static_scalars;
    std::map<std::string, ColorSpec> static_colors;
    std::map<std::string, std::string> static_strings;
    
    // Animated parameters (keyframed)
    std::map<std::string, AnimatedScalarSpec> animated_scalars;
    std::map<std::string, AnimatedColorSpec> animated_colors;
    
    /**
     * @brief Evaluate effect parameters at a specific time.
     * @param time_seconds Time in seconds to evaluate.
     * @return EffectSpec with evaluated values at the given time.
     */
    [[nodiscard]] TACHYON_API EffectSpec evaluate(double time_seconds) const;
    
    /**
     * @brief Check if this effect has any animated parameters.
     */
    [[nodiscard]] bool is_animated() const noexcept {
        return !animated_scalars.empty() || !animated_colors.empty();
    }
};

// Backward compatibility: allow using EffectSpec where AnimatedEffectSpec is expected
inline EffectSpec EffectSpec::evaluate(double /*time_seconds*/) const {
    return *this;
}

} // namespace tachyon

#ifdef _MSC_VER
#pragma warning(pop)
#endif
