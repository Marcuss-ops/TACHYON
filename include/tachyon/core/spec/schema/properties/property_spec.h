#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/keyframe.h"
#include "tachyon/renderer2d/raster/path/path_types.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/template_spec.h"

#include <vector>
#include <optional>
#include <string>
#include <utility>

namespace tachyon {

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
    renderer2d::MaskPath value;
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier;
    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct AnimatedMaskPathSpec {
    std::optional<renderer2d::MaskPath> value;
    std::vector<MaskPathKeyframeSpec> keyframes;
    std::optional<std::string> expression;
    WiggleSpec wiggle;

    AnimatedMaskPathSpec() = default;
    AnimatedMaskPathSpec(const renderer2d::MaskPath& v) : value(v) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
};

// JSON serialization is handled in property_spec_serialize_helpers.h and .cpp files.

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
    [[nodiscard]] EffectSpec evaluate(double time_seconds) const {
        EffectSpec result;
        result.enabled = enabled;
        result.type = type;
        
        // Start with static values
        result.scalars = static_scalars;
        result.colors = static_colors;
        result.strings = static_strings;
        
        // Override with animated values at the given time
        for (const auto& [key, anim] : animated_scalars) {
            if (!anim.keyframes.empty()) {
                // Find the two keyframes surrounding the time
                double value = anim.keyframes.front().value; // default to first
                for (size_t i = 0; i < anim.keyframes.size() - 1; ++i) {
                    const auto& kf1 = anim.keyframes[i];
                    const auto& kf2 = anim.keyframes[i + 1];
                    if (time_seconds >= kf1.time && time_seconds <= kf2.time) {
                        // Linear interpolation for now (could use easing)
                        double t = (time_seconds - kf1.time) / (kf2.time - kf1.time);
                        value = kf1.value + t * (kf2.value - kf1.value);
                        break;
                    } else if (time_seconds < kf1.time) {
                        value = kf1.value;
                        break;
                    } else if (time_seconds > kf2.time && i == anim.keyframes.size() - 2) {
                        value = kf2.value;
                    }
                }
                result.scalars[key] = value;
            } else if (anim.value.has_value()) {
                result.scalars[key] = anim.value.value();
            }
        }
        
        for (const auto& [key, anim] : animated_colors) {
            if (!anim.keyframes.empty()) {
                // Find the two keyframes surrounding the time
                ColorSpec value = anim.keyframes.front().value; // default to first
                for (size_t i = 0; i < anim.keyframes.size() - 1; ++i) {
                    const auto& kf1 = anim.keyframes[i];
                    const auto& kf2 = anim.keyframes[i + 1];
                    if (time_seconds >= kf1.time && time_seconds <= kf2.time) {
                        // Linear interpolation for each channel
                        double t = (time_seconds - kf1.time) / (kf2.time - kf1.time);
                        value.r = static_cast<uint8_t>(kf1.value.r + t * (kf2.value.r - kf1.value.r));
                        value.g = static_cast<uint8_t>(kf1.value.g + t * (kf2.value.g - kf1.value.g));
                        value.b = static_cast<uint8_t>(kf1.value.b + t * (kf2.value.b - kf1.value.b));
                        value.a = static_cast<uint8_t>(kf1.value.a + t * (kf2.value.a - kf1.value.a));
                        break;
                    } else if (time_seconds < kf1.time) {
                        value = kf1.value;
                        break;
                    } else if (time_seconds > kf2.time && i == anim.keyframes.size() - 2) {
                        value = kf2.value;
                    }
                }
                result.colors[key] = value;
            } else if (anim.value.has_value()) {
                result.colors[key] = anim.value.value();
            }
        }
        
        return result;
    }
    
    /**
     * @brief Check if this effect has any animated parameters.
     */
    [[nodiscard]] bool is_animated() const noexcept {
        return !animated_scalars.empty() || !animated_colors.empty();
    }
};

void to_json(nlohmann::json& j, const AnimatedEffectSpec& e);
void from_json(const nlohmann::json& j, AnimatedEffectSpec& e);

// Backward compatibility: allow using EffectSpec where AnimatedEffectSpec is expected
inline EffectSpec EffectSpec::evaluate(double /*time_seconds*/) const {
    return *this;
}

} // namespace tachyon
