#include "tachyon/core/spec/property_spec_serialize_helpers.h"

using json = nlohmann::json;

namespace tachyon {

// ColorKeyframeSpec serialization
void to_json(json& j, const ColorKeyframeSpec& k) {
    j["time"] = k.time;
    j["value"] = k.value;
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["spring"] = json{{"stiffness", k.spring.stiffness}, {"damping", k.spring.damping}, {"mass", k.spring.mass}, {"velocity", k.spring.velocity}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

void from_json(const json& j, ColorKeyframeSpec& k) {
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

// AnimatedColorSpec serialization
void to_json(json& j, const AnimatedColorSpec& a) {
    if (a.value.has_value()) j["value"] = a.value.value();
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
}

void from_json(const json& j, AnimatedColorSpec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        ColorSpec c;
        from_json(j.at("value"), c);
        a.value = c;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<ColorKeyframeSpec>>();
    }
}

// WiggleSpec serialization
void to_json(json& j, const WiggleSpec& w) {
    j["enabled"] = w.enabled;
    j["frequency"] = w.frequency;
    j["amplitude"] = w.amplitude;
    j["seed"] = w.seed;
    j["octaves"] = w.octaves;
}

void from_json(const json& j, WiggleSpec& w) {
    if (j.contains("enabled") && j.at("enabled").is_boolean()) w.enabled = j.at("enabled").get<bool>();
    if (j.contains("frequency") && j.at("frequency").is_number()) w.frequency = j.at("frequency").get<double>();
    if (j.contains("amplitude") && j.at("amplitude").is_number()) w.amplitude = j.at("amplitude").get<double>();
    if (j.contains("seed") && j.at("seed").is_number()) w.seed = j.at("seed").get<uint64_t>();
    if (j.contains("octaves") && j.at("octaves").is_number()) w.octaves = j.at("octaves").get<int>();
}

// AnimatedEffectSpec serialization
void to_json(json& j, const AnimatedEffectSpec& e) {
    j["type"] = e.type;
    j["enabled"] = e.enabled;
    
    if (!e.static_scalars.empty()) {
        json scalars_obj;
        for (const auto& [key, val] : e.static_scalars) {
            scalars_obj[key] = val;
        }
        j["scalars"] = scalars_obj;
    }
    if (!e.static_colors.empty()) {
        json colors_obj;
        for (const auto& [key, val] : e.static_colors) {
            colors_obj[key] = val;
        }
        j["colors"] = colors_obj;
    }
    if (!e.static_strings.empty()) {
        json strings_obj;
        for (const auto& [key, val] : e.static_strings) {
            strings_obj[key] = val;
        }
        j["strings"] = strings_obj;
    }
    
    // Animated parameters
    if (!e.animated_scalars.empty()) {
        json anim_scalars_obj;
        for (const auto& [key, val] : e.animated_scalars) {
            anim_scalars_obj[key] = val;
        }
        j["animated_scalars"] = anim_scalars_obj;
    }
    if (!e.animated_colors.empty()) {
        json anim_colors_obj;
        for (const auto& [key, val] : e.animated_colors) {
            anim_colors_obj[key] = val;
        }
        j["animated_colors"] = anim_colors_obj;
    }
}

void from_json(const json& j, AnimatedEffectSpec& e) {
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

// AnimatedEffectSpec::evaluate implementation
EffectSpec AnimatedEffectSpec::evaluate(double time_seconds) const {
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

} // namespace tachyon
