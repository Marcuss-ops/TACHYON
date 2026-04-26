#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
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

// JSON serialization for keyframe types
inline void to_json(nlohmann::json& j, const ScalarKeyframeSpec& k) {
    j["time"] = k.time;
    j["value"] = k.value;
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = nlohmann::json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["spring"] = nlohmann::json{{"stiffness", k.spring.stiffness}, {"damping", k.spring.damping}, {"mass", k.spring.mass}, {"velocity", k.spring.velocity}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

inline void from_json(const nlohmann::json& j, ScalarKeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_number()) k.value = j.at("value").get<double>();
    if (j.contains("easing") && j.at("easing").is_number()) k.easing = static_cast<animation::EasingPreset>(j.at("easing").get<int>());
    if (j.contains("bezier") && j.at("bezier").is_object()) {
        auto& b = j.at("bezier");
        if (b.contains("cx1") && b.at("cx1").is_number()) k.bezier.cx1 = b.at("cx1").get<double>();
        if (b.contains("cy1") && b.at("cy1").is_number()) k.bezier.cy1 = b.at("cy1").get<double>();
        if (b.contains("cx2") && b.at("cx2").is_number()) k.bezier.cx2 = b.at("cx2").get<double>();
        if (b.contains("cy2") && b.at("cy2").is_number()) k.bezier.cy2 = b.at("cy2").get<double>();
    }
    if (j.contains("spring") && j.at("spring").is_object()) {
        auto& s = j.at("spring");
        if (s.contains("stiffness") && s.at("stiffness").is_number()) k.spring.stiffness = s.at("stiffness").get<double>();
        if (s.contains("damping") && s.at("damping").is_number()) k.spring.damping = s.at("damping").get<double>();
        if (s.contains("mass") && s.at("mass").is_number()) k.spring.mass = s.at("mass").get<double>();
        if (s.contains("velocity") && s.at("velocity").is_number()) k.spring.velocity = s.at("velocity").get<double>();
    }
    if (j.contains("speed_in") && j.at("speed_in").is_number()) k.speed_in = j.at("speed_in").get<double>();
    if (j.contains("influence_in") && j.at("influence_in").is_number()) k.influence_in = j.at("influence_in").get<double>();
    if (j.contains("speed_out") && j.at("speed_out").is_number()) k.speed_out = j.at("speed_out").get<double>();
    if (j.contains("influence_out") && j.at("influence_out").is_number()) k.influence_out = j.at("influence_out").get<double>();
}

inline void to_json(nlohmann::json& j, const Vector2KeyframeSpec& k) {
    j["time"] = k.time;
    j["value"] = nlohmann::json{{"x", k.value.x}, {"y", k.value.y}};
    j["tangent_in"] = nlohmann::json{{"x", k.tangent_in.x}, {"y", k.tangent_in.y}};
    j["tangent_out"] = nlohmann::json{{"x", k.tangent_out.x}, {"y", k.tangent_out.y}};
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = nlohmann::json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["spring"] = nlohmann::json{{"stiffness", k.spring.stiffness}, {"damping", k.spring.damping}, {"mass", k.spring.mass}, {"velocity", k.spring.velocity}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

inline void from_json(const nlohmann::json& j, Vector2KeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        if (v.contains("x") && v.at("x").is_number()) k.value.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.value.y = v.at("y").get<float>();
    }
    if (j.contains("tangent_in") && j.at("tangent_in").is_object()) {
        auto& v = j.at("tangent_in");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_in.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_in.y = v.at("y").get<float>();
    }
    if (j.contains("tangent_out") && j.at("tangent_out").is_object()) {
        auto& v = j.at("tangent_out");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_out.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_out.y = v.at("y").get<float>();
    }
    if (j.contains("easing") && j.at("easing").is_number()) k.easing = static_cast<animation::EasingPreset>(j.at("easing").get<int>());
    if (j.contains("bezier") && j.at("bezier").is_object()) {
        auto& b = j.at("bezier");
        if (b.contains("cx1") && b.at("cx1").is_number()) k.bezier.cx1 = b.at("cx1").get<double>();
        if (b.contains("cy1") && b.at("cy1").is_number()) k.bezier.cy1 = b.at("cy1").get<double>();
        if (b.contains("cx2") && b.at("cx2").is_number()) k.bezier.cx2 = b.at("cx2").get<double>();
        if (b.contains("cy2") && b.at("cy2").is_number()) k.bezier.cy2 = b.at("cy2").get<double>();
    }
    if (j.contains("spring") && j.at("spring").is_object()) {
        auto& s = j.at("spring");
        if (s.contains("stiffness") && s.at("stiffness").is_number()) k.spring.stiffness = s.at("stiffness").get<double>();
        if (s.contains("damping") && s.at("damping").is_number()) k.spring.damping = s.at("damping").get<double>();
        if (s.contains("mass") && s.at("mass").is_number()) k.spring.mass = s.at("mass").get<double>();
        if (s.contains("velocity") && s.at("velocity").is_number()) k.spring.velocity = s.at("velocity").get<double>();
    }
    if (j.contains("speed_in") && j.at("speed_in").is_number()) k.speed_in = j.at("speed_in").get<double>();
    if (j.contains("influence_in") && j.at("influence_in").is_number()) k.influence_in = j.at("influence_in").get<double>();
    if (j.contains("speed_out") && j.at("speed_out").is_number()) k.speed_out = j.at("speed_out").get<double>();
    if (j.contains("influence_out") && j.at("influence_out").is_number()) k.influence_out = j.at("influence_out").get<double>();
}

inline void to_json(nlohmann::json& j, const ColorKeyframeSpec& k) {
    j["time"] = k.time;
    j["value"] = k.value;
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = nlohmann::json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["spring"] = nlohmann::json{{"stiffness", k.spring.stiffness}, {"damping", k.spring.damping}, {"mass", k.spring.mass}, {"velocity", k.spring.velocity}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

inline void from_json(const nlohmann::json& j, ColorKeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) from_json(j.at("value"), k.value);
    if (j.contains("easing") && j.at("easing").is_number()) k.easing = static_cast<animation::EasingPreset>(j.at("easing").get<int>());
    if (j.contains("bezier") && j.at("bezier").is_object()) {
        auto& b = j.at("bezier");
        if (b.contains("cx1") && b.at("cx1").is_number()) k.bezier.cx1 = b.at("cx1").get<double>();
        if (b.contains("cy1") && b.at("cy1").is_number()) k.bezier.cy1 = b.at("cy1").get<double>();
        if (b.contains("cx2") && b.at("cx2").is_number()) k.bezier.cx2 = b.at("cx2").get<double>();
        if (b.contains("cy2") && b.at("cy2").is_number()) k.bezier.cy2 = b.at("cy2").get<double>();
    }
    if (j.contains("spring") && j.at("spring").is_object()) {
        auto& s = j.at("spring");
        if (s.contains("stiffness") && s.at("stiffness").is_number()) k.spring.stiffness = s.at("stiffness").get<double>();
        if (s.contains("damping") && s.at("damping").is_number()) k.spring.damping = s.at("damping").get<double>();
        if (s.contains("mass") && s.at("mass").is_number()) k.spring.mass = s.at("mass").get<double>();
        if (s.contains("velocity") && s.at("velocity").is_number()) k.spring.velocity = s.at("velocity").get<double>();
    }
    if (j.contains("speed_in") && j.at("speed_in").is_number()) k.speed_in = j.at("speed_in").get<double>();
    if (j.contains("influence_in") && j.at("influence_in").is_number()) k.influence_in = j.at("influence_in").get<double>();
    if (j.contains("speed_out") && j.at("speed_out").is_number()) k.speed_out = j.at("speed_out").get<double>();
    if (j.contains("influence_out") && j.at("influence_out").is_number()) k.influence_out = j.at("influence_out").get<double>();
}

// JSON serialization for animated types
inline void to_json(nlohmann::json& j, const AnimatedScalarSpec& a) {
    if (a.value.has_value()) j["value"] = a.value.value();
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
    if (a.audio_band.has_value()) j["audio_band"] = static_cast<int>(a.audio_band.value());
    if (a.audio_min != 0.0) j["audio_min"] = a.audio_min;
    if (a.audio_max != 1.0) j["audio_max"] = a.audio_max;
    if (a.expression.has_value()) j["expression"] = a.expression.value();
}

inline void from_json(const nlohmann::json& j, AnimatedScalarSpec& a) {
    if (j.contains("value") && j.at("value").is_number()) a.value = j.at("value").get<double>();
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<ScalarKeyframeSpec>>();
    }
    if (j.contains("audio_band") && j.at("audio_band").is_number()) a.audio_band = static_cast<AudioBandType>(j.at("audio_band").get<int>());
    if (j.contains("audio_min") && j.at("audio_min").is_number()) a.audio_min = j.at("audio_min").get<double>();
    if (j.contains("audio_max") && j.at("audio_max").is_number()) a.audio_max = j.at("audio_max").get<double>();
    if (j.contains("expression") && j.at("expression").is_string()) a.expression = j.at("expression").get<std::string>();
}

inline void to_json(nlohmann::json& j, const AnimatedVector2Spec& a) {
    if (a.value.has_value()) j["value"] = nlohmann::json{{"x", a.value.value().x}, {"y", a.value.value().y}};
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
    if (a.expression.has_value()) j["expression"] = a.expression.value();
}

inline void from_json(const nlohmann::json& j, AnimatedVector2Spec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        math::Vector2 val;
        if (v.contains("x") && v.at("x").is_number()) val.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) val.y = v.at("y").get<float>();
        a.value = val;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<Vector2KeyframeSpec>>();
    }
    if (j.contains("expression") && j.at("expression").is_string()) a.expression = j.at("expression").get<std::string>();
}

inline void to_json(nlohmann::json& j, const AnimatedVector3Spec::Keyframe& k) {
    j["time"] = k.time;
    j["value"] = nlohmann::json{{"x", k.value.x}, {"y", k.value.y}, {"z", k.value.z}};
    j["tangent_in"] = nlohmann::json{{"x", k.tangent_in.x}, {"y", k.tangent_in.y}, {"z", k.tangent_in.z}};
    j["tangent_out"] = nlohmann::json{{"x", k.tangent_out.x}, {"y", k.tangent_out.y}, {"z", k.tangent_out.z}};
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = nlohmann::json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

inline void from_json(const nlohmann::json& j, AnimatedVector3Spec::Keyframe& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        if (v.contains("x") && v.at("x").is_number()) k.value.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.value.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) k.value.z = v.at("z").get<float>();
    }
    if (j.contains("tangent_in") && j.at("tangent_in").is_object()) {
        auto& v = j.at("tangent_in");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_in.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_in.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) k.tangent_in.z = v.at("z").get<float>();
    }
    if (j.contains("tangent_out") && j.at("tangent_out").is_object()) {
        auto& v = j.at("tangent_out");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_out.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_out.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) k.tangent_out.z = v.at("z").get<float>();
    }
    if (j.contains("easing") && j.at("easing").is_number()) k.easing = static_cast<animation::EasingPreset>(j.at("easing").get<int>());
    if (j.contains("bezier") && j.at("bezier").is_object()) {
        auto& b = j.at("bezier");
        if (b.contains("cx1") && b.at("cx1").is_number()) k.bezier.cx1 = b.at("cx1").get<double>();
        if (b.contains("cy1") && b.at("cy1").is_number()) k.bezier.cy1 = b.at("cy1").get<double>();
        if (b.contains("cx2") && b.at("cx2").is_number()) k.bezier.cx2 = b.at("cx2").get<double>();
        if (b.contains("cy2") && b.at("cy2").is_number()) k.bezier.cy2 = b.at("cy2").get<double>();
    }
    if (j.contains("speed_in") && j.at("speed_in").is_number()) k.speed_in = j.at("speed_in").get<double>();
    if (j.contains("influence_in") && j.at("influence_in").is_number()) k.influence_in = j.at("influence_in").get<double>();
    if (j.contains("speed_out") && j.at("speed_out").is_number()) k.speed_out = j.at("speed_out").get<double>();
    if (j.contains("influence_out") && j.at("influence_out").is_number()) k.influence_out = j.at("influence_out").get<double>();
}

inline void to_json(nlohmann::json& j, const AnimatedVector3Spec& a) {
    if (a.value.has_value()) j["value"] = nlohmann::json{{"x", a.value.value().x}, {"y", a.value.value().y}, {"z", a.value.value().z}};
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
    if (a.expression.has_value()) j["expression"] = a.expression.value();
}

inline void from_json(const nlohmann::json& j, AnimatedVector3Spec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        math::Vector3 val;
        if (v.contains("x") && v.at("x").is_number()) val.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) val.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) val.z = v.at("z").get<float>();
        a.value = val;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<AnimatedVector3Spec::Keyframe>>();
    }
    if (j.contains("expression") && j.at("expression").is_string()) a.expression = j.at("expression").get<std::string>();
}

inline void to_json(nlohmann::json& j, const AnimatedColorSpec& a) {
    if (a.value.has_value()) j["value"] = a.value.value();
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
}

inline void from_json(const nlohmann::json& j, AnimatedColorSpec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        ColorSpec c;
        from_json(j.at("value"), c);
        a.value = c;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<ColorKeyframeSpec>>();
    }
}

// JSON serialization for MaskPathKeyframeSpec
inline void to_json(nlohmann::json& j, const MaskPathKeyframeSpec& k) {
    j["time"] = k.time;
    // Serialize MaskPath value
    nlohmann::json vertices_array = nlohmann::json::array();
    for (const auto& v : k.value.vertices) {
        nlohmann::json v_obj;
        v_obj["position"] = nlohmann::json{{"x", v.position.x}, {"y", v.position.y}};
        v_obj["in_tangent"] = nlohmann::json{{"x", v.in_tangent.x}, {"y", v.in_tangent.y}};
        v_obj["out_tangent"] = nlohmann::json{{"x", v.out_tangent.x}, {"y", v.out_tangent.y}};
        v_obj["feather_inner"] = v.feather_inner;
        v_obj["feather_outer"] = v.feather_outer;
        vertices_array.push_back(v_obj);
    }
    j["value"] = nlohmann::json{{"vertices", vertices_array}, {"is_closed", k.value.is_closed}, {"is_inverted", k.value.is_inverted}};
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = nlohmann::json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["spring"] = nlohmann::json{{"stiffness", k.spring.stiffness}, {"damping", k.spring.damping}, {"mass", k.spring.mass}, {"velocity", k.spring.velocity}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

inline void from_json(const nlohmann::json& j, MaskPathKeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        if (v.contains("vertices") && v.at("vertices").is_array()) {
            for (auto& vert : v.at("vertices")) {
                renderer2d::MaskVertex mv;
                if (vert.contains("position") && vert.at("position").is_object()) {
                    auto& p = vert.at("position");
                    if (p.contains("x") && p.at("x").is_number()) mv.position.x = p.at("x").get<float>();
                    if (p.contains("y") && p.at("y").is_number()) mv.position.y = p.at("y").get<float>();
                }
                if (vert.contains("in_tangent") && vert.at("in_tangent").is_object()) {
                    auto& t = vert.at("in_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.in_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.in_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("out_tangent") && vert.at("out_tangent").is_object()) {
                    auto& t = vert.at("out_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.out_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.out_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("feather_inner") && vert.at("feather_inner").is_number()) mv.feather_inner = vert.at("feather_inner").get<float>();
                if (vert.contains("feather_outer") && vert.at("feather_outer").is_number()) mv.feather_outer = vert.at("feather_outer").get<float>();
                k.value.vertices.push_back(mv);
            }
        }
        if (v.contains("is_closed") && v.at("is_closed").is_boolean()) k.value.is_closed = v.at("is_closed").get<bool>();
        if (v.contains("is_inverted") && v.at("is_inverted").is_boolean()) k.value.is_inverted = v.at("is_inverted").get<bool>();
    }
    if (j.contains("easing") && j.at("easing").is_number()) k.easing = static_cast<animation::EasingPreset>(j.at("easing").get<int>());
    if (j.contains("bezier") && j.at("bezier").is_object()) {
        auto& b = j.at("bezier");
        if (b.contains("cx1") && b.at("cx1").is_number()) k.bezier.cx1 = b.at("cx1").get<double>();
        if (b.contains("cy1") && b.at("cy1").is_number()) k.bezier.cy1 = b.at("cy1").get<double>();
        if (b.contains("cx2") && b.at("cx2").is_number()) k.bezier.cx2 = b.at("cx2").get<double>();
        if (b.contains("cy2") && b.at("cy2").is_number()) k.bezier.cy2 = b.at("cy2").get<double>();
    }
    if (j.contains("spring") && j.at("spring").is_object()) {
        auto& s = j.at("spring");
        if (s.contains("stiffness") && s.at("stiffness").is_number()) k.spring.stiffness = s.at("stiffness").get<double>();
        if (s.contains("damping") && s.at("damping").is_number()) k.spring.damping = s.at("damping").get<double>();
        if (s.contains("mass") && s.at("mass").is_number()) k.spring.mass = s.at("mass").get<double>();
        if (s.contains("velocity") && s.at("velocity").is_number()) k.spring.velocity = s.at("velocity").get<double>();
    }
    if (j.contains("speed_in") && j.at("speed_in").is_number()) k.speed_in = j.at("speed_in").get<double>();
    if (j.contains("influence_in") && j.at("influence_in").is_number()) k.influence_in = j.at("influence_in").get<double>();
    if (j.contains("speed_out") && j.at("speed_out").is_number()) k.speed_out = j.at("speed_out").get<double>();
    if (j.contains("influence_out") && j.at("influence_out").is_number()) k.influence_out = j.at("influence_out").get<double>();
}

// JSON serialization for AnimatedMaskPathSpec
inline void to_json(nlohmann::json& j, const AnimatedMaskPathSpec& a) {
    if (a.value.has_value()) {
        nlohmann::json vertices_array = nlohmann::json::array();
        for (const auto& v : a.value.value().vertices) {
            nlohmann::json v_obj;
            v_obj["position"] = nlohmann::json{{"x", v.position.x}, {"y", v.position.y}};
            v_obj["in_tangent"] = nlohmann::json{{"x", v.in_tangent.x}, {"y", v.in_tangent.y}};
            v_obj["out_tangent"] = nlohmann::json{{"x", v.out_tangent.x}, {"y", v.out_tangent.y}};
            v_obj["feather_inner"] = v.feather_inner;
            v_obj["feather_outer"] = v.feather_outer;
            vertices_array.push_back(v_obj);
        }
        j["value"] = nlohmann::json{{"vertices", vertices_array}, {"is_closed", a.value.value().is_closed}, {"is_inverted", a.value.value().is_inverted}};
    }
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
}

inline void from_json(const nlohmann::json& j, AnimatedMaskPathSpec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        renderer2d::MaskPath mp;
        auto& v = j.at("value");
        if (v.contains("vertices") && v.at("vertices").is_array()) {
            for (auto& vert : v.at("vertices")) {
                renderer2d::MaskVertex mv;
                if (vert.contains("position") && vert.at("position").is_object()) {
                    auto& p = vert.at("position");
                    if (p.contains("x") && p.at("x").is_number()) mv.position.x = p.at("x").get<float>();
                    if (p.contains("y") && p.at("y").is_number()) mv.position.y = p.at("y").get<float>();
                }
                if (vert.contains("in_tangent") && vert.at("in_tangent").is_object()) {
                    auto& t = vert.at("in_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.in_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.in_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("out_tangent") && vert.at("out_tangent").is_object()) {
                    auto& t = vert.at("out_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.out_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.out_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("feather_inner") && vert.at("feather_inner").is_number()) mv.feather_inner = vert.at("feather_inner").get<float>();
                if (vert.contains("feather_outer") && vert.at("feather_outer").is_number()) mv.feather_outer = vert.at("feather_outer").get<float>();
                mp.vertices.push_back(mv);
            }
        }
        if (v.contains("is_closed") && v.at("is_closed").is_boolean()) mp.is_closed = v.at("is_closed").get<bool>();
        if (v.contains("is_inverted") && v.at("is_inverted").is_boolean()) mp.is_inverted = v.at("is_inverted").get<bool>();
        a.value = mp;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<MaskPathKeyframeSpec>>();
    }
}

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

inline void to_json(nlohmann::json& j, const AnimatedEffectSpec& e) {
    j["type"] = e.type;
    j["enabled"] = e.enabled;
    
    if (!e.static_scalars.empty()) {
        nlohmann::json scalars_obj;
        for (const auto& [key, val] : e.static_scalars) {
            scalars_obj[key] = val;
        }
        j["scalars"] = scalars_obj;
    }
    if (!e.static_colors.empty()) {
        nlohmann::json colors_obj;
        for (const auto& [key, val] : e.static_colors) {
            colors_obj[key] = val;
        }
        j["colors"] = colors_obj;
    }
    if (!e.static_strings.empty()) {
        nlohmann::json strings_obj;
        for (const auto& [key, val] : e.static_strings) {
            strings_obj[key] = val;
        }
        j["strings"] = strings_obj;
    }
    
    // Animated parameters
    if (!e.animated_scalars.empty()) {
        nlohmann::json anim_scalars_obj;
        for (const auto& [key, val] : e.animated_scalars) {
            anim_scalars_obj[key] = val;
        }
        j["animated_scalars"] = anim_scalars_obj;
    }
    if (!e.animated_colors.empty()) {
        nlohmann::json anim_colors_obj;
        for (const auto& [key, val] : e.animated_colors) {
            anim_colors_obj[key] = val;
        }
        j["animated_colors"] = anim_colors_obj;
    }
}

inline void from_json(const nlohmann::json& j, AnimatedEffectSpec& e) {
    if (j.contains("type") && j.at("type").is_string()) e.type = j.at("type").get<std::string>();
    if (j.contains("enabled") && j.at("enabled").is_boolean()) e.enabled = j.at("enabled").get<bool>();
    
    // Static values
    if (j.contains("scalars") && j.at("scalars").is_object()) {
        for (auto& [key, val] : j.at("scalars").items()) {
            if (val.is_number()) e.static_scalars[key] = val.get<double>();
        }
    }
    if (j.contains("colors") && j.at("colors").is_object()) {
        for (auto& [key, val] : j.at("colors").items()) {
            val.get_to(e.static_colors[key]);
        }
    }
    if (j.contains("strings") && j.at("strings").is_object()) {
        for (auto& [key, val] : j.at("strings").items()) {
            if (val.is_string()) e.static_strings[key] = val.get<std::string>();
        }
    }
    
    // Animated values
    if (j.contains("animated_scalars") && j.at("animated_scalars").is_object()) {
        for (auto& [key, val] : j.at("animated_scalars").items()) {
            val.get_to(e.animated_scalars[key]);
        }
    }
    if (j.contains("animated_colors") && j.at("animated_colors").is_object()) {
        for (auto& [key, val] : j.at("animated_colors").items()) {
            val.get_to(e.animated_colors[key]);
        }
    }
}

// Backward compatibility: allow using EffectSpec where AnimatedEffectSpec is expected
inline EffectSpec EffectSpec::evaluate(double /*time_seconds*/) const {
    return *this;
}

} // namespace tachyon
