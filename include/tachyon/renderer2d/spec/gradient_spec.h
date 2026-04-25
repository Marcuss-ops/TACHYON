#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <cstdint>
#include <vector>

namespace tachyon {

struct GradientStop {
    float offset{0.0f};
    ColorSpec color{255, 255, 255, 255};
};

struct AnimatedGradientStop {
    AnimatedScalarSpec offset;
    AnimatedColorSpec color;
};

enum class GradientType {
    Linear,
    Radial
};

struct GradientSpec {
    GradientType type{GradientType::Linear};
    math::Vector2 start{0.0f, 0.0f};
    math::Vector2 end{100.0f, 0.0f};
    float radial_radius{100.0f};
    std::vector<GradientStop> stops;
};

/**
 * @brief Animated gradient specification with keyframable stops, angle, and position.
 * All parameters can be animated over time using keyframes or expressions.
 */
struct AnimatedGradientSpec {
    std::string type{"linear"}; // "linear" | "radial"
    
    // For linear gradients
    AnimatedScalarSpec angle; // Rotation angle in degrees
    
    // For radial gradients  
    AnimatedVector2Spec center; // Center point for radial
    AnimatedScalarSpec radius; // Radius for radial
    
    // Gradient stops with animated colors and positions
    std::vector<AnimatedGradientStop> stops;
    
    /**
     * @brief Evaluate the animated gradient at a specific time.
     * @param time_seconds Time in seconds to evaluate.
     * @return A static GradientSpec for the given frame.
     */
    [[nodiscard]] GradientSpec evaluate(double time_seconds) const {
        GradientSpec result;
        result.type = (type == "radial") ? GradientType::Radial : GradientType::Linear;
        
        // Sample animated properties
        if (!angle.keyframes.empty()) {
            // Find surrounding keyframes and interpolate
            double angle_val = angle.value.has_value() ? angle.value.value() : 0.0;
            // Simplified - just use first keyframe value
            if (!angle.keyframes.empty()) {
                angle_val = angle.keyframes.front().value;
                for (std::size_t i = 0; i < angle.keyframes.size() - 1; ++i) {
                    const auto& kf1 = angle.keyframes[i];
                    const auto& kf2 = angle.keyframes[i + 1];
                    if (time_seconds >= kf1.time && time_seconds <= kf2.time) {
                        double t = (time_seconds - kf1.time) / (kf2.time - kf1.time);
                        angle_val = kf1.value + t * (kf2.value - kf1.value);
                        break;
                    }
                }
            }
            
            // Convert angle to start/end points for linear gradient
            float rad = static_cast<float>(angle_val * 3.141592653589793 / 180.0);
            float len = 100.0f;
            result.start = math::Vector2(-std::cos(rad) * len, -std::sin(rad) * len);
            result.end = math::Vector2(std::cos(rad) * len, std::sin(rad) * len);
        }
        
        if (!center.keyframes.empty() && !center.value.has_value()) {
            result.start = center.keyframes.front().value;
        } else if (center.value.has_value()) {
            result.start = center.value.value();
        }
        
        if (!radius.keyframes.empty()) {
            result.radial_radius = static_cast<float>(radius.keyframes.front().value);
        } else if (radius.value.has_value()) {
            result.radial_radius = static_cast<float>(radius.value.value());
        }
        
        // Sample stops
        for (const auto& anim_stop : stops) {
            GradientStop stop;
            
            // Sample offset
            if (!anim_stop.offset.keyframes.empty()) {
                stop.offset = static_cast<float>(anim_stop.offset.keyframes.front().value);
            } else if (anim_stop.offset.value.has_value()) {
                stop.offset = static_cast<float>(anim_stop.offset.value.value());
            }
            
            // Sample color
            if (!anim_stop.color.keyframes.empty()) {
                stop.color = anim_stop.color.keyframes.front().value;
            } else if (anim_stop.color.value.has_value()) {
                stop.color = anim_stop.color.value.value();
            }
            
            result.stops.push_back(stop);
        }
        
        return result;
    }
    
    [[nodiscard]] bool empty() const noexcept {
        return stops.empty() && angle.empty() && center.empty() && radius.empty();
    }
};

} // namespace tachyon

namespace tachyon {

// JSON serialization for GradientStop
inline void to_json(nlohmann::json& j, const GradientStop& s) {
    j = nlohmann::json{{"offset", s.offset}, {"color", s.color}};
}

inline void from_json(const nlohmann::json& j, GradientStop& s) {
    if (j.contains("offset") && j.at("offset").is_number()) s.offset = j.at("offset").get<float>();
    if (j.contains("color") && j.at("color").is_object()) from_json(j.at("color"), s.color);
}

// JSON serialization for AnimatedGradientStop
inline void to_json(nlohmann::json& j, const AnimatedGradientStop& s) {
    j = nlohmann::json{{"offset", s.offset}, {"color", s.color}};
}

inline void from_json(const nlohmann::json& j, AnimatedGradientStop& s) {
    if (j.contains("offset")) s.offset = j.at("offset").get<AnimatedScalarSpec>();
    if (j.contains("color")) s.color = j.at("color").get<AnimatedColorSpec>();
}

// JSON serialization for GradientSpec
inline void to_json(nlohmann::json& j, const GradientSpec& g) {
    j["type"] = (g.type == GradientType::Radial) ? "radial" : "linear";
    j["start"] = nlohmann::json{{"x", g.start.x}, {"y", g.start.y}};
    j["end"] = nlohmann::json{{"x", g.end.x}, {"y", g.end.y}};
    j["radial_radius"] = g.radial_radius;
    j["stops"] = g.stops;
}

inline void from_json(const nlohmann::json& j, GradientSpec& g) {
    if (j.contains("type") && j.at("type").is_string()) {
        std::string t = j.at("type").get<std::string>();
        g.type = (t == "radial") ? GradientType::Radial : GradientType::Linear;
    }
    if (j.contains("start") && j.at("start").is_object()) {
        auto& s = j.at("start");
        if (s.contains("x") && s.at("x").is_number()) g.start.x = s.at("x").get<float>();
        if (s.contains("y") && s.at("y").is_number()) g.start.y = s.at("y").get<float>();
    }
    if (j.contains("end") && j.at("end").is_object()) {
        auto& e = j.at("end");
        if (e.contains("x") && e.at("x").is_number()) g.end.x = e.at("x").get<float>();
        if (e.contains("y") && e.at("y").is_number()) g.end.y = e.at("y").get<float>();
    }
    if (j.contains("radial_radius") && j.at("radial_radius").is_number()) g.radial_radius = j.at("radial_radius").get<float>();
    if (j.contains("stops") && j.at("stops").is_array()) g.stops = j.at("stops").get<std::vector<GradientStop>>();
}

// JSON serialization for AnimatedGradientSpec
inline void to_json(nlohmann::json& j, const AnimatedGradientSpec& g) {
    j["type"] = g.type;
    if (!g.angle.empty()) j["angle"] = g.angle;
    if (!g.center.empty()) j["center"] = g.center;
    if (!g.radius.empty()) j["radius"] = g.radius;
    if (!g.stops.empty()) j["stops"] = g.stops;
}

inline void from_json(const nlohmann::json& j, AnimatedGradientSpec& g) {
    if (j.contains("type") && j.at("type").is_string()) g.type = j.at("type").get<std::string>();
    if (j.contains("angle")) g.angle = j.at("angle").get<AnimatedScalarSpec>();
    if (j.contains("center")) g.center = j.at("center").get<AnimatedVector2Spec>();
    if (j.contains("radius")) g.radius = j.at("radius").get<AnimatedScalarSpec>();
    if (j.contains("stops") && j.at("stops").is_array()) g.stops = j.at("stops").get<std::vector<AnimatedGradientStop>>();
}

} // namespace tachyon
