#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <algorithm>

using json = nlohmann::json;

namespace tachyon {

bool read_string(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_string()) return false;
    out = value.get<std::string>();
    return true;
}

bool read_bool(const json& object, const char* key, bool& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_boolean()) return false;
    out = value.get<bool>();
    return true;
}

bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number_integer()) return false;
    out = value.get<std::int64_t>();
    return true;
}

bool read_vector2_like(const json& value, math::Vector2& out) {
    if (value.is_array()) {
        if (value.size() < 2 || !value[0].is_number() || !value[1].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float scalar = static_cast<float>(value.get<double>());
        out = {scalar, scalar};
        return true;
    }
    return false;
}

bool read_vector2_object(const json& value, math::Vector2& out) {
    if (!value.is_object()) return false;
    if (!value.contains("x") || !value.contains("y") || !value.at("x").is_number() || !value.at("y").is_number()) return false;
    out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()) };
    return true;
}

bool parse_vector2_value(const json& value, math::Vector2& out) {
    return read_vector2_like(value, out) || read_vector2_object(value, out);
}

bool read_vector3_like(const json& value, math::Vector3& out) {
    if (value.is_array()) {
        if (value.size() < 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()), static_cast<float>(value[2].get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float s = static_cast<float>(value.get<double>());
        out = {s, s, s};
        return true;
    }
    return false;
}

bool read_vector3_object(const json& value, math::Vector3& out) {
    if (!value.is_object()) return false;
    if (!value.contains("x") || !value.contains("y") || !value.contains("z") ||
        !value.at("x").is_number() || !value.at("y").is_number() || !value.at("z").is_number()) return false;
    out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()), static_cast<float>(value.at("z").get<double>()) };
    return true;
}

bool parse_vector3_value(const json& value, math::Vector3& out) {
    return read_vector3_like(value, out) || read_vector3_object(value, out);
}

bool parse_gradient_spec(const json& object, GradientSpec& out) {
    if (!object.is_object()) return false;
    
    std::string type_str;
    if (read_string(object, "type", type_str)) {
        out.type = (type_str == "radial") ? GradientType::Radial : GradientType::Linear;
    }
    
    if (object.contains("start")) parse_vector2_value(object.at("start"), out.start);
    if (object.contains("end")) parse_vector2_value(object.at("end"), out.end);
    
    if (object.contains("radial_radius") && object.at("radial_radius").is_number()) {
        out.radial_radius = static_cast<float>(object.at("radial_radius").get<double>());
    }

    if (object.contains("stops") && object.at("stops").is_array()) {
        out.stops.clear();
        for (const auto& stop_json : object.at("stops")) {
            if (stop_json.is_object()) {
                GradientStop stop;
                if (stop_json.contains("offset") && stop_json.at("offset").is_number()) {
                    stop.offset = static_cast<float>(stop_json.at("offset").get<double>());
                }
                if (stop_json.contains("color")) {
                    parse_color_value(stop_json.at("color"), stop.color);
                }
                out.stops.push_back(stop);
            }
        }
    }
    return true;
}

bool parse_color_value(const json& value, ColorSpec& out) {
    if (value.is_array()) {
        if (value.size() < 3) return false;
        out.r = static_cast<std::uint8_t>(std::clamp(value[0].get<int>(), 0, 255));
        out.g = static_cast<std::uint8_t>(std::clamp(value[1].get<int>(), 0, 255));
        out.b = static_cast<std::uint8_t>(std::clamp(value[2].get<int>(), 0, 255));
        out.a = (value.size() >= 4) ? static_cast<std::uint8_t>(std::clamp(value[3].get<int>(), 0, 255)) : 255;
        return true;
    }
    if (value.is_string()) {
        std::string hex = value.get<std::string>();
        if (hex.empty()) return false;
        if (hex[0] == '#') hex = hex.substr(1);
        
        uint32_t r, g, b, a = 255;
        if (hex.length() == 6) {
            std::sscanf(hex.c_str(), "%2x%2x%2x", &r, &g, &b);
        } else if (hex.length() == 8) {
            std::sscanf(hex.c_str(), "%2x%2x%2x%2x", &r, &g, &b, &a);
        } else if (hex.length() == 3) {
            std::sscanf(hex.c_str(), "%1x%1x%1x", &r, &g, &b);
            r = (r << 4) | r;
            g = (g << 4) | g;
            b = (b << 4) | b;
        } else {
            return false;
        }
        out.r = static_cast<std::uint8_t>(r);
        out.g = static_cast<std::uint8_t>(g);
        out.b = static_cast<std::uint8_t>(b);
        out.a = static_cast<std::uint8_t>(a);
        return true;
    }
    if (value.is_object()) {
        if (value.contains("r") && value.contains("g") && value.contains("b")) {
            out.r = static_cast<std::uint8_t>(std::clamp(value.at("r").get<int>(), 0, 255));
            out.g = static_cast<std::uint8_t>(std::clamp(value.at("g").get<int>(), 0, 255));
            out.b = static_cast<std::uint8_t>(std::clamp(value.at("b").get<int>(), 0, 255));
            out.a = (value.contains("a")) ? static_cast<std::uint8_t>(std::clamp(value.at("a").get<int>(), 0, 255)) : 255;
            return true;
        }
    }
    return false;
}

animation::EasingPreset parse_easing_preset(const json& value) {
    if (!value.is_string()) return animation::EasingPreset::None;
    const std::string easing = value.get<std::string>();
    if (easing == "ease_in" || easing == "easeIn") return animation::EasingPreset::EaseIn;
    if (easing == "ease_out" || easing == "easeOut") return animation::EasingPreset::EaseOut;
    if (easing == "ease_in_out" || easing == "easeInOut") return animation::EasingPreset::EaseInOut;
    if (easing == "custom" || easing == "bezier") return animation::EasingPreset::Custom;
    return animation::EasingPreset::None;
}

animation::CubicBezierEasing parse_bezier(const json& value) {
    animation::CubicBezierEasing bezier = animation::CubicBezierEasing::linear();
    if (!value.is_object()) return bezier;
    if (value.contains("cx1") && value.at("cx1").is_number()) bezier.cx1 = value.at("cx1").get<double>();
    if (value.contains("cy1") && value.at("cy1").is_number()) bezier.cy1 = value.at("cy1").get<double>();
    if (value.contains("cx2") && value.at("cx2").is_number()) bezier.cx2 = value.at("cx2").get<double>();
    if (value.contains("cy2") && value.at("cy2").is_number()) bezier.cy2 = value.at("cy2").get<double>();
    return bezier;
}

renderer2d::LineCap parse_line_cap(const json& value) {
    if (!value.is_string()) return renderer2d::LineCap::Butt;
    std::string s = value.get<std::string>();
    if (s == "round") return renderer2d::LineCap::Round;
    if (s == "square") return renderer2d::LineCap::Square;
    return renderer2d::LineCap::Butt;
}

renderer2d::LineJoin parse_line_join(const json& value) {
    if (!value.is_string()) return renderer2d::LineJoin::Miter;
    std::string s = value.get<std::string>();
    if (s == "round") return renderer2d::LineJoin::Round;
    if (s == "bevel") return renderer2d::LineJoin::Bevel;
    return renderer2d::LineJoin::Miter;
}

std::optional<AudioBandType> parse_audio_band_type(const json& value) {
    if (!value.is_string()) return std::nullopt;
    const std::string band = value.get<std::string>();
    if (band == "bass") return AudioBandType::Bass;
    if (band == "mid") return AudioBandType::Mid;
    if (band == "high") return AudioBandType::High;
    if (band == "presence") return AudioBandType::Presence;
    if (band == "rms") return AudioBandType::Rms;
    return std::nullopt;
}

} // namespace tachyon
