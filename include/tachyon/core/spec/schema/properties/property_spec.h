#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/math/vector2.h"
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
#include <variant>

namespace tachyon {

struct ScalarKeyframeSpec;
struct Vector2KeyframeSpec;
struct ColorKeyframeSpec;
struct AnimatedScalarSpec;
struct AnimatedVector2Spec;
struct AnimatedColorSpec;
struct AnimatedMaskPathSpec;

/**
 * @brief Unified variant for any effect or property parameter.
 */
using ParameterValue = std::variant<
    double,
    bool,
    std::string,
    ColorSpec,
    math::Vector2,
    AnimatedScalarSpec,
    AnimatedColorSpec,
    AnimatedVector2Spec,
    AnimatedMaskPathSpec
>;

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

} // namespace tachyon

#ifdef _MSC_VER
#pragma warning(pop)
#endif
