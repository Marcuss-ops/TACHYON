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

// JSON serialization declarations (implementations in gradient_spec_serialize.cpp)
void to_json(nlohmann::json& j, const GradientStop& s);
void from_json(const nlohmann::json& j, GradientStop& s);
void to_json(nlohmann::json& j, const AnimatedGradientStop& s);
void from_json(const nlohmann::json& j, AnimatedGradientStop& s);
void to_json(nlohmann::json& j, const GradientSpec& g);
void from_json(const nlohmann::json& j, GradientSpec& g);
void to_json(nlohmann::json& j, const AnimatedGradientSpec& g);
void from_json(const nlohmann::json& j, AnimatedGradientSpec& g);

} // namespace tachyon
