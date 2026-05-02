#include "tachyon/core/spec/parsing/spec_value_parsers.h"
#include <algorithm>

namespace tachyon {

bool parse_vector2_value(const json& value, math::Vector2& out) {
    if (value.is_array()) {
        if (value.size() < 2 || !value[0].is_number() || !value[1].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()) };
        return true;
    }
    if (value.is_object()) {
        if (!value.contains("x") || !value.contains("y") || !value.at("x").is_number() || !value.at("y").is_number()) return false;
        out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float scalar = static_cast<float>(value.get<double>());
        out = {scalar, scalar};
        return true;
    }
    return false;
}

bool parse_vector3_value(const json& value, math::Vector3& out) {
    if (value.is_array()) {
        if (value.size() < 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()), static_cast<float>(value[2].get<double>()) };
        return true;
    }
    if (value.is_object()) {
        if (!value.contains("x") || !value.contains("y") || !value.contains("z") ||
            !value.at("x").is_number() || !value.at("y").is_number() || !value.at("z").is_number()) return false;
        out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()), static_cast<float>(value.at("z").get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float s = static_cast<float>(value.get<double>());
        out = {s, s, s};
        return true;
    }
    return false;
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
    if (value.is_object()) {
        if (value.contains("r") && value.contains("g") && value.contains("b")) {
            out.r = static_cast<std::uint8_t>(std::clamp(value.at("r").get<int>(), 0, 255));
            out.g = static_cast<std::uint8_t>(std::clamp(value.at("g").get<int>(), 0, 255));
            out.b = static_cast<std::uint8_t>(std::clamp(value.at("b").get<int>(), 0, 255));
            out.a = (value.contains("a")) ? static_cast<std::uint8_t>(std::clamp(value.at("a").get<int>(), 0, 255)) : 255;
            return true;
        }
    }
    if (value.is_string()) {
        std::string hex = value.get<std::string>();
        if (hex.size() >= 4 && hex[0] == '#') {
            try {
                if (hex.size() == 4) { // #RGB
                    out.r = static_cast<std::uint8_t>(std::stoi(hex.substr(1, 1), nullptr, 16) * 17);
                    out.g = static_cast<std::uint8_t>(std::stoi(hex.substr(2, 1), nullptr, 16) * 17);
                    out.b = static_cast<std::uint8_t>(std::stoi(hex.substr(3, 1), nullptr, 16) * 17);
                    out.a = 255;
                    return true;
                } else if (hex.size() == 7) { // #RRGGBB
                    out.r = static_cast<std::uint8_t>(std::stoi(hex.substr(1, 2), nullptr, 16));
                    out.g = static_cast<std::uint8_t>(std::stoi(hex.substr(3, 2), nullptr, 16));
                    out.b = static_cast<std::uint8_t>(std::stoi(hex.substr(5, 2), nullptr, 16));
                    out.a = 255;
                    return true;
                } else if (hex.size() == 9) { // #RRGGBBAA
                    out.r = static_cast<std::uint8_t>(std::stoi(hex.substr(1, 2), nullptr, 16));
                    out.g = static_cast<std::uint8_t>(std::stoi(hex.substr(3, 2), nullptr, 16));
                    out.b = static_cast<std::uint8_t>(std::stoi(hex.substr(5, 2), nullptr, 16));
                    out.a = static_cast<std::uint8_t>(std::stoi(hex.substr(7, 2), nullptr, 16));
                    return true;
                }
            } catch (...) {
                return false;
            }
        }
    }
    return false;
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
