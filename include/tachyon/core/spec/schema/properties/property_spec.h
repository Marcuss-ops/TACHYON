#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/keyframe.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/renderer2d/path/mask_path.h"

#include <vector>
#include <optional>
#include <string>

namespace tachyon {

struct ScalarKeyframeSpec {
    double time{0.0};
    double value{0.0};
    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    animation::SpringEasing spring{}; ///< Spring parameters (used when easing == Spring)

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

    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    animation::SpringEasing spring{}; ///< Spring parameters (used when easing == Spring)

    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct ColorKeyframeSpec {
    double time{0.0};
    ColorSpec value{255, 255, 255, 255};
    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    animation::SpringEasing spring{}; ///< Spring parameters (used when easing == Spring)

    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct AnimatedScalarSpec {
    std::optional<double> value;
    std::vector<ScalarKeyframeSpec> keyframes;
    std::optional<AudioBandType> audio_band;
    double audio_min{0.0};
    double audio_max{1.0};
    std::optional<std::string> expression;

    AnimatedScalarSpec() = default;
    explicit AnimatedScalarSpec(double val) : value(val) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !audio_band.has_value() && !expression.has_value();
    }
};

struct AnimatedVector2Spec {
    std::optional<math::Vector2> value;
    std::vector<Vector2KeyframeSpec> keyframes;
    std::optional<std::string> expression;

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

        animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
        animation::EasingPreset easing{animation::EasingPreset::None};
        animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
        animation::SpringEasing spring{}; ///< Spring parameters (used when easing == Spring)

        double speed_in{0.0};
        double influence_in{33.333333333};
        double speed_out{0.0};
        double influence_out{33.333333333};
    };
    std::vector<Keyframe> keyframes;
    std::optional<std::string> expression;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !expression.has_value();
    }
};

struct AnimatedColorSpec {
    std::optional<ColorSpec> value;
    std::vector<ColorKeyframeSpec> keyframes;

    AnimatedColorSpec() = default;
    explicit AnimatedColorSpec(const ColorSpec& val) : value(val) {}

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
};

// Mask path keyframe for animated mask paths
struct MaskPathKeyframeSpec {
    double time{0.0};
    renderer2d::MaskPath value;
    animation::InterpolationMode interpolation{animation::InterpolationMode::Linear};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    animation::SpringEasing spring{}; ///< Spring parameters (used when easing == Spring)
    
    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

// Animated mask path with keyframe support
struct AnimatedMaskPathSpec {
    std::optional<renderer2d::MaskPath> value;
    std::vector<MaskPathKeyframeSpec> keyframes;
    
    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
    
    /**
     * @brief Evaluate the mask path at a specific time.
     * @param time_seconds Time in seconds.
     * @return Evaluated MaskPath (with morphing applied if keyframes present).
     */
    [[nodiscard]] renderer2d::MaskPath evaluate(double time_seconds) const {
        if (keyframes.empty()) {
            return value.value_or(renderer2d::MaskPath{});
        }
        
        // Find surrounding keyframes
        if (time_seconds <= keyframes.front().time) {
            return keyframes.front().value;
        }
        if (time_seconds >= keyframes.back().time) {
            return keyframes.back().value;
        }
        
        for (size_t i = 0; i < keyframes.size() - 1; ++i) {
            const auto& kf1 = keyframes[i];
            const auto& kf2 = keyframes[i + 1];
            
            if (time_seconds >= kf1.time && time_seconds <= kf2.time) {
                // Hold key: no interpolation
                if (kf1.interpolation == animation::InterpolationMode::Hold) {
                    return kf1.value;
                }

                double t = (time_seconds - kf1.time) / (kf2.time - kf1.time);
                t = std::clamp(t, 0.0, 1.0);
                
                // Apply easing if not None
                if (kf1.easing != animation::EasingPreset::None) {
                    t = animation::apply_easing(t, kf1.easing, kf1.bezier, kf1.spring);
                }
                
                // Interpolate between the two mask paths
                renderer2d::MaskPath result = kf1.value;
                const auto& v1 = kf1.value.vertices;
                const auto& v2 = kf2.value.vertices;
                size_t num_vertices = std::min(v1.size(), v2.size());
                
                if (num_vertices > 0) {
                    result.vertices.clear();
                    result.vertices.reserve(num_vertices);
                    for (size_t j = 0; j < num_vertices; ++j) {
                        renderer2d::MaskVertex v;
                        v.position.x = v1[j].position.x + (v2[j].position.x - v1[j].position.x) * static_cast<float>(t);
                        v.position.y = v1[j].position.y + (v2[j].position.y - v1[j].position.y) * static_cast<float>(t);
                        v.in_tangent.x = v1[j].in_tangent.x + (v2[j].in_tangent.x - v1[j].in_tangent.x) * static_cast<float>(t);
                        v.in_tangent.y = v1[j].in_tangent.y + (v2[j].in_tangent.y - v1[j].in_tangent.y) * static_cast<float>(t);
                        v.out_tangent.x = v1[j].out_tangent.x + (v2[j].out_tangent.x - v1[j].out_tangent.x) * static_cast<float>(t);
                        v.out_tangent.y = v1[j].out_tangent.y + (v2[j].out_tangent.y - v1[j].out_tangent.y) * static_cast<float>(t);
                        v.feather_inner = v1[j].feather_inner + (v2[j].feather_inner - v1[j].feather_inner) * static_cast<float>(t);
                        v.feather_outer = v1[j].feather_outer + (v2[j].feather_outer - v1[j].feather_outer) * static_cast<float>(t);
                        result.vertices.push_back(v);
                    }
                }
                result.is_closed = kf1.value.is_closed;
                result.is_inverted = kf1.value.is_inverted;
                return result;
            }
        }
        
        return keyframes.back().value;
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
    [[nodiscard]] EffectSpec evaluate(double time_seconds) const;
    
    /**
     * @brief Check if this effect has any animated parameters.
     */
    [[nodiscard]] bool is_animated() const noexcept {
        return !animated_scalars.empty() || !animated_colors.empty();
    }
};

// JSON serialization declarations (implementations in property_spec_serialize.cpp)
void to_json(nlohmann::json& j, const ScalarKeyframeSpec& k);
void from_json(const nlohmann::json& j, ScalarKeyframeSpec& k);
void to_json(nlohmann::json& j, const Vector2KeyframeSpec& k);
void from_json(const nlohmann::json& j, Vector2KeyframeSpec& k);
void to_json(nlohmann::json& j, const ColorKeyframeSpec& k);
void from_json(const nlohmann::json& j, ColorKeyframeSpec& k);
void to_json(nlohmann::json& j, const AnimatedVector3Spec::Keyframe& k);
void from_json(const nlohmann::json& j, AnimatedVector3Spec::Keyframe& k);
void to_json(nlohmann::json& j, const AnimatedVector3Spec& a);
void from_json(const nlohmann::json& j, AnimatedVector3Spec& a);
void to_json(nlohmann::json& j, const AnimatedScalarSpec& a);
void from_json(const nlohmann::json& j, AnimatedScalarSpec& a);
void to_json(nlohmann::json& j, const AnimatedVector2Spec& a);
void from_json(const nlohmann::json& j, AnimatedVector2Spec& a);
void to_json(nlohmann::json& j, const AnimatedColorSpec& a);
void from_json(const nlohmann::json& j, AnimatedColorSpec& a);
void to_json(nlohmann::json& j, const MaskPathKeyframeSpec& k);
void from_json(const nlohmann::json& j, MaskPathKeyframeSpec& k);
void to_json(nlohmann::json& j, const AnimatedMaskPathSpec& a);
void from_json(const nlohmann::json& j, AnimatedMaskPathSpec& a);
void to_json(nlohmann::json& j, const AnimatedEffectSpec& e);
void from_json(const nlohmann::json& j, AnimatedEffectSpec& e);

// Backward compatibility: allow using EffectSpec where AnimatedEffectSpec is expected
inline EffectSpec EffectSpec::evaluate(double /*time_seconds*/) const {
    return *this;
}

} // namespace tachyon
