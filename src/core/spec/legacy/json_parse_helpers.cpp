#include "tachyon/core/spec/legacy/json_parse_helpers.h"
#include <algorithm>

namespace tachyon {

bool parse_gradient_spec(const json& object, GradientSpec& out) {
    if (!object.is_object()) return false;
    
    std::string type_str;
    if (read_string(object, "type", type_str)) {
        out.type = (type_str == "radial") ? GradientType::Radial : GradientType::Linear;
    }
    
    if (object.contains("start")) read_vector2(object.at("start"), out.start);
    if (object.contains("end")) read_vector2(object.at("end"), out.end);
    
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
                    read_color(stop_json.at("color"), stop.color);
                }
                out.stops.push_back(stop);
            }
        }
    }
    return true;
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
