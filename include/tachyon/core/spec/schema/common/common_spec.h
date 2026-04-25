#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include <string>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>

namespace tachyon {

struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    [[nodiscard]] math::Vector3 to_vector3() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f};
    }
};

inline void to_json(nlohmann::json& j, const ColorSpec& c) {
    j = nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
}

inline void from_json(const nlohmann::json& j, ColorSpec& c) {
    if (j.contains("r") && j.at("r").is_number()) c.r = j.at("r").get<std::uint8_t>();
    if (j.contains("g") && j.at("g").is_number()) c.g = j.at("g").get<std::uint8_t>();
    if (j.contains("b") && j.at("b").is_number()) c.b = j.at("b").get<std::uint8_t>();
    if (j.contains("a") && j.at("a").is_number()) c.a = j.at("a").get<std::uint8_t>();
}

enum class TrackMatteType {
    None,
    Alpha,
    AlphaInverted,
    Luma,
    LumaInverted
};

enum class LayerType {
    Solid,
    Shape,
    Image,
    Video,
    Text,
    Camera,
    Precomp,
    Light,
    Mask,
    NullLayer,
    Unknown
};

enum class AudioBandType {
    Bass,
    Mid,
    High,
    Presence,
    Rms
};

struct EffectSpec {
    bool enabled{true};
    std::string type;
    std::map<std::string, double> scalars;
    std::map<std::string, ColorSpec> colors;
    std::map<std::string, std::string> strings;
    
    // Easing for effect progress (e.g., transition t)
    animation::EasingPreset easing_preset{animation::EasingPreset::None};
    animation::CubicBezierEasing custom_easing{animation::CubicBezierEasing::linear()};

    [[nodiscard]] EffectSpec evaluate(double time_seconds) const;
};

inline void to_json(nlohmann::json& j, const EffectSpec& e) {
    j["type"] = e.type;
    j["enabled"] = e.enabled;
    if (!e.scalars.empty()) {
        nlohmann::json scalars_obj;
        for (const auto& [key, val] : e.scalars) {
            scalars_obj[key] = val;
        }
        j["scalars"] = scalars_obj;
    }
    if (!e.colors.empty()) {
        nlohmann::json colors_obj;
        for (const auto& [key, val] : e.colors) {
            colors_obj[key] = val;
        }
        j["colors"] = colors_obj;
    }
    if (!e.strings.empty()) {
        nlohmann::json strings_obj;
        for (const auto& [key, val] : e.strings) {
            strings_obj[key] = val;
        }
        j["strings"] = strings_obj;
    }
    // Serialize easing
    j["easing_preset"] = static_cast<int>(e.easing_preset);
    if (e.easing_preset == animation::EasingPreset::Custom) {
        j["custom_easing"] = nlohmann::json{
            {"cx1", e.custom_easing.cx1},
            {"cy1", e.custom_easing.cy1},
            {"cx2", e.custom_easing.cx2},
            {"cy2", e.custom_easing.cy2}
        };
    }
}

inline void from_json(const nlohmann::json& j, EffectSpec& e) {
    if (j.contains("type") && j.at("type").is_string()) e.type = j.at("type").get<std::string>();
    if (j.contains("enabled") && j.at("enabled").is_boolean()) e.enabled = j.at("enabled").get<bool>();
    if (j.contains("scalars") && j.at("scalars").is_object()) {
        for (auto& [key, val] : j.at("scalars").items()) {
            if (val.is_number()) e.scalars[key] = val.get<double>();
        }
    }
    if (j.contains("colors") && j.at("colors").is_object()) {
        for (auto& [key, val] : j.at("colors").items()) {
            if (val.is_object()) {
                ColorSpec c;
                from_json(val, c);
                e.colors[key] = c;
            }
        }
    }
    if (j.contains("strings") && j.at("strings").is_object()) {
        for (auto& [key, val] : j.at("strings").items()) {
            if (val.is_string()) e.strings[key] = val.get<std::string>();
        }
    }
    // Deserialize easing
    if (j.contains("easing_preset") && j.at("easing_preset").is_number()) {
        e.easing_preset = static_cast<animation::EasingPreset>(j.at("easing_preset").get<int>());
    }
    if (e.easing_preset == animation::EasingPreset::Custom && j.contains("custom_easing") && j.at("custom_easing").is_object()) {
        const auto& bezier = j.at("custom_easing");
        if (bezier.contains("cx1") && bezier.at("cx1").is_number()) e.custom_easing.cx1 = bezier.at("cx1").get<double>();
        if (bezier.contains("cy1") && bezier.at("cy1").is_number()) e.custom_easing.cy1 = bezier.at("cy1").get<double>();
        if (bezier.contains("cx2") && bezier.at("cx2").is_number()) e.custom_easing.cx2 = bezier.at("cx2").get<double>();
        if (bezier.contains("cy2") && bezier.at("cy2").is_number()) e.custom_easing.cy2 = bezier.at("cy2").get<double>();
    }
}

} // namespace tachyon
