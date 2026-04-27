#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/color/color_grading_controls.h"

#include <string>

namespace tachyon {

/**
 * @brief Color grading specification with animated properties.
 * 
 * Supports keyframe animation for exposure, contrast, saturation,
 * and color-based properties (lift, gamma, gain).
 */
struct ColorGradingSpec {
    // Scalar animated properties
    AnimatedScalarSpec exposure{0.0};
    AnimatedScalarSpec contrast{1.0};
    AnimatedScalarSpec saturation{1.0};
    
    // Color animated properties
    AnimatedColorSpec lift;   // Color for shadows/midtones/highlights adjustment
    AnimatedColorSpec gamma;  // Gamma adjustment per channel
    AnimatedColorSpec gain;   // Gain adjustment per channel
    
    // Optional: for secondary grading (hue/saturation/luminance curves)
    bool has_secondary{false};
    std::string secondary_curve_json; // JSON string for complex curves (stub for now)
    
    [[nodiscard]] bool empty() const noexcept {
        return exposure.empty() && contrast.empty() && saturation.empty() &&
               lift.empty() && gamma.empty() && gain.empty();
    }
    
    /**
     * @brief Evaluate all animated properties at a specific time.
     * @param time_seconds Time in seconds.
     * @return Evaluated PrimaryGrading values.
     */
    [[nodiscard]] color::PrimaryGrading evaluate(double time_seconds) const {
        color::PrimaryGrading result;
        
        // Evaluate scalar properties
        if (!exposure.empty()) {
            // Sample exposure at time
            if (!exposure.keyframes.empty()) {
                result.exposure = sample_animated_scalar(exposure, time_seconds);
            } else if (exposure.value.has_value()) {
                result.exposure = exposure.value.value();
            }
        }
        
        if (!contrast.empty()) {
            if (!contrast.keyframes.empty()) {
                result.contrast = sample_animated_scalar(contrast, time_seconds);
            } else if (contrast.value.has_value()) {
                result.contrast = contrast.value.value();
            }
        }
        
        if (!saturation.empty()) {
            if (!saturation.keyframes.empty()) {
                result.saturation = sample_animated_scalar(saturation, time_seconds);
            } else if (saturation.value.has_value()) {
                result.saturation = saturation.value.value();
            }
        }
        
        // Evaluate color properties
        if (!lift.empty()) {
            if (!lift.keyframes.empty()) {
                ColorSpec c = sample_animated_color(lift, time_seconds);
                result.lift[0] = c.r / 255.0;
                result.lift[1] = c.g / 255.0;
                result.lift[2] = c.b / 255.0;
            } else if (lift.value.has_value()) {
                const auto& c = lift.value.value();
                result.lift[0] = c.r / 255.0;
                result.lift[1] = c.g / 255.0;
                result.lift[2] = c.b / 255.0;
            }
        }
        
        if (!gamma.empty()) {
            if (!gamma.keyframes.empty()) {
                ColorSpec c = sample_animated_color(gamma, time_seconds);
                result.gamma[0] = c.r / 255.0;
                result.gamma[1] = c.g / 255.0;
                result.gamma[2] = c.b / 255.0;
            } else if (gamma.value.has_value()) {
                const auto& c = gamma.value.value();
                result.gamma[0] = c.r / 255.0;
                result.gamma[1] = c.g / 255.0;
                result.gamma[2] = c.b / 255.0;
            }
        }
        
        if (!gain.empty()) {
            if (!gain.keyframes.empty()) {
                ColorSpec c = sample_animated_color(gain, time_seconds);
                result.gain[0] = c.r / 255.0;
                result.gain[1] = c.g / 255.0;
                result.gain[2] = c.b / 255.0;
            } else if (gain.value.has_value()) {
                const auto& c = gain.value.value();
                result.gain[0] = c.r / 255.0;
                result.gain[1] = c.g / 255.0;
                result.gain[2] = c.b / 255.0;
            }
        }
        
        return result;
    }
    
private:
    /**
     * @brief Helper to sample an AnimatedScalarSpec at a given time.
     */
    static double sample_animated_scalar(const AnimatedScalarSpec& spec, double time_seconds) {
        if (spec.keyframes.empty()) {
            return spec.value.value_or(0.0);
        }
        
        // Find surrounding keyframes
        if (time_seconds <= spec.keyframes.front().time) {
            return spec.keyframes.front().value;
        }
        if (time_seconds >= spec.keyframes.back().time) {
            return spec.keyframes.back().value;
        }
        
        for (size_t i = 0; i < spec.keyframes.size() - 1; ++i) {
            const auto& kf1 = spec.keyframes[i];
            const auto& kf2 = spec.keyframes[i + 1];
            
            if (time_seconds >= kf1.time && time_seconds <= kf2.time) {
                double t = (time_seconds - kf1.time) / (kf2.time - kf1.time);
                t = std::clamp(t, 0.0, 1.0);
                
                // Apply easing if not None
                if (kf1.easing != animation::EasingPreset::None) {
                    t = animation::apply_easing(t, kf1.easing, kf1.bezier, kf1.spring);
                }
                
                return kf1.value + t * (kf2.value - kf1.value);
            }
        }
        
        return spec.keyframes.back().value;
    }
    
    /**
     * @brief Helper to sample an AnimatedColorSpec at a given time.
     */
    static ColorSpec sample_animated_color(const AnimatedColorSpec& spec, double time_seconds) {
        if (spec.keyframes.empty()) {
            return spec.value.value_or(ColorSpec{0, 0, 0, 255});
        }
        
        // Find surrounding keyframes
        if (time_seconds <= spec.keyframes.front().time) {
            return spec.keyframes.front().value;
        }
        if (time_seconds >= spec.keyframes.back().time) {
            return spec.keyframes.back().value;
        }
        
        for (size_t i = 0; i < spec.keyframes.size() - 1; ++i) {
            const auto& kf1 = spec.keyframes[i];
            const auto& kf2 = spec.keyframes[i + 1];
            
            if (time_seconds >= kf1.time && time_seconds <= kf2.time) {
                double t = (time_seconds - kf1.time) / (kf2.time - kf1.time);
                t = std::clamp(t, 0.0, 1.0);
                
                // Apply easing if not None
                if (kf1.easing != animation::EasingPreset::None) {
                    t = animation::apply_easing(t, kf1.easing, kf1.bezier, kf1.spring);
                }
                
                ColorSpec result;
                result.r = static_cast<uint8_t>(kf1.value.r + t * (kf2.value.r - kf1.value.r));
                result.g = static_cast<uint8_t>(kf1.value.g + t * (kf2.value.g - kf1.value.g));
                result.b = static_cast<uint8_t>(kf1.value.b + t * (kf2.value.b - kf1.value.b));
                result.a = static_cast<uint8_t>(kf1.value.a + t * (kf2.value.a - kf1.value.a));
                return result;
            }
        }
        
        return spec.keyframes.back().value;
    }
};

// JSON serialization for ColorGradingSpec
inline void to_json(nlohmann::json& j, const ColorGradingSpec& c) {
    if (!c.exposure.empty()) j["exposure"] = c.exposure;
    if (!c.contrast.empty()) j["contrast"] = c.contrast;
    if (!c.saturation.empty()) j["saturation"] = c.saturation;
    if (!c.lift.empty()) j["lift"] = c.lift;
    if (!c.gamma.empty()) j["gamma"] = c.gamma;
    if (!c.gain.empty()) j["gain"] = c.gain;
    if (c.has_secondary) j["has_secondary"] = c.has_secondary;
    if (!c.secondary_curve_json.empty()) j["secondary_curve_json"] = c.secondary_curve_json;
}

inline void from_json(const nlohmann::json& j, ColorGradingSpec& c) {
    if (j.contains("exposure") && j.at("exposure").is_object()) {
        c.exposure = j.at("exposure").get<AnimatedScalarSpec>();
    }
    if (j.contains("contrast") && j.at("contrast").is_object()) {
        c.contrast = j.at("contrast").get<AnimatedScalarSpec>();
    }
    if (j.contains("saturation") && j.at("saturation").is_object()) {
        c.saturation = j.at("saturation").get<AnimatedScalarSpec>();
    }
    if (j.contains("lift") && j.at("lift").is_object()) {
        c.lift = j.at("lift").get<AnimatedColorSpec>();
    }
    if (j.contains("gamma") && j.at("gamma").is_object()) {
        c.gamma = j.at("gamma").get<AnimatedColorSpec>();
    }
    if (j.contains("gain") && j.at("gain").is_object()) {
        c.gain = j.at("gain").get<AnimatedColorSpec>();
    }
    if (j.contains("has_secondary") && j.at("has_secondary").is_boolean()) {
        c.has_secondary = j.at("has_secondary").get<bool>();
    }
    if (j.contains("secondary_curve_json") && j.at("secondary_curve_json").is_string()) {
        c.secondary_curve_json = j.at("secondary_curve_json").get<std::string>();
    }
}

} // namespace tachyon
