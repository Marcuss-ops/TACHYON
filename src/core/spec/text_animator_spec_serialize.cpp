#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

void to_json(json& j, const TextAnimatorPropertySpec& p) {
    j = json{};
    if (p.opacity_value.has_value()) j["opacity_value"] = *p.opacity_value;
    if (!p.opacity_keyframes.empty()) j["opacity_keyframes"] = json(p.opacity_keyframes);
    if (p.position_offset_value.has_value()) j["position_offset_value"] = json(*p.position_offset_value);
    if (!p.position_offset_keyframes.empty()) j["position_offset_keyframes"] = json(p.position_offset_keyframes);
    if (p.scale_value.has_value()) j["scale_value"] = *p.scale_value;
    if (!p.scale_keyframes.empty()) j["scale_keyframes"] = json(p.scale_keyframes);
    if (p.rotation_value.has_value()) j["rotation_value"] = *p.rotation_value;
    if (!p.rotation_keyframes.empty()) j["rotation_keyframes"] = json(p.rotation_keyframes);
    if (p.tracking_amount_value.has_value()) j["tracking_amount_value"] = *p.tracking_amount_value;
    if (!p.tracking_amount_keyframes.empty()) j["tracking_amount_keyframes"] = json(p.tracking_amount_keyframes);
    if (p.fill_color_value.has_value()) j["fill_color_value"] = json(*p.fill_color_value);
    if (!p.fill_color_keyframes.empty()) j["fill_color_keyframes"] = json(p.fill_color_keyframes);
    if (p.stroke_color_value.has_value()) j["stroke_color_value"] = json(*p.stroke_color_value);
    if (!p.stroke_color_keyframes.empty()) j["stroke_color_keyframes"] = json(p.stroke_color_keyframes);
    if (p.stroke_width_value.has_value()) j["stroke_width_value"] = *p.stroke_width_value;
    if (!p.stroke_width_keyframes.empty()) j["stroke_width_keyframes"] = json(p.stroke_width_keyframes);
    if (p.blur_radius_value.has_value()) j["blur_radius_value"] = *p.blur_radius_value;
    if (!p.blur_radius_keyframes.empty()) j["blur_radius_keyframes"] = json(p.blur_radius_keyframes);
    if (p.reveal_value.has_value()) j["reveal_value"] = *p.reveal_value;
    if (!p.reveal_keyframes.empty()) j["reveal_keyframes"] = json(p.reveal_keyframes);
}

void from_json(const json& j, TextAnimatorPropertySpec& p) {
    if (j.contains("opacity_value") && j.at("opacity_value").is_number()) p.opacity_value = j.at("opacity_value").get<double>();
    if (j.contains("opacity_keyframes") && j.at("opacity_keyframes").is_array()) p.opacity_keyframes = j.at("opacity_keyframes").get<std::vector<ScalarKeyframeSpec>>();
    if (j.contains("position_offset_value") && j.at("position_offset_value").is_object()) p.position_offset_value = j.at("position_offset_value").get<math::Vector2>();
    if (j.contains("position_offset_keyframes") && j.at("position_offset_keyframes").is_array()) p.position_offset_keyframes = j.at("position_offset_keyframes").get<std::vector<Vector2KeyframeSpec>>();
    if (j.contains("scale_value") && j.at("scale_value").is_number()) p.scale_value = j.at("scale_value").get<double>();
    if (j.contains("scale_keyframes") && j.at("scale_keyframes").is_array()) p.scale_keyframes = j.at("scale_keyframes").get<std::vector<ScalarKeyframeSpec>>();
    if (j.contains("rotation_value") && j.at("rotation_value").is_number()) p.rotation_value = j.at("rotation_value").get<double>();
    if (j.contains("rotation_keyframes") && j.at("rotation_keyframes").is_array()) p.rotation_keyframes = j.at("rotation_keyframes").get<std::vector<ScalarKeyframeSpec>>();
    if (j.contains("tracking_amount_value") && j.at("tracking_amount_value").is_number()) p.tracking_amount_value = j.at("tracking_amount_value").get<double>();
    if (j.contains("tracking_amount_keyframes") && j.at("tracking_amount_keyframes").is_array()) p.tracking_amount_keyframes = j.at("tracking_amount_keyframes").get<std::vector<ScalarKeyframeSpec>>();
    if (j.contains("fill_color_value") && j.at("fill_color_value").is_object()) p.fill_color_value = j.at("fill_color_value").get<ColorSpec>();
    if (j.contains("fill_color_keyframes") && j.at("fill_color_keyframes").is_array()) p.fill_color_keyframes = j.at("fill_color_keyframes").get<std::vector<ColorKeyframeSpec>>();
    if (j.contains("stroke_color_value") && j.at("stroke_color_value").is_object()) p.stroke_color_value = j.at("stroke_color_value").get<ColorSpec>();
    if (j.contains("stroke_color_keyframes") && j.at("stroke_color_keyframes").is_array()) p.stroke_color_keyframes = j.at("stroke_color_keyframes").get<std::vector<ColorKeyframeSpec>>();
    if (j.contains("stroke_width_value") && j.at("stroke_width_value").is_number()) p.stroke_width_value = j.at("stroke_width_value").get<double>();
    if (j.contains("stroke_width_keyframes") && j.at("stroke_width_keyframes").is_array()) p.stroke_width_keyframes = j.at("stroke_width_keyframes").get<std::vector<ScalarKeyframeSpec>>();
    if (j.contains("blur_radius_value") && j.at("blur_radius_value").is_number()) p.blur_radius_value = j.at("blur_radius_value").get<double>();
    if (j.contains("blur_radius_keyframes") && j.at("blur_radius_keyframes").is_array()) p.blur_radius_keyframes = j.at("blur_radius_keyframes").get<std::vector<ScalarKeyframeSpec>>();
    if (j.contains("reveal_value") && j.at("reveal_value").is_number()) p.reveal_value = j.at("reveal_value").get<double>();
    if (j.contains("reveal_keyframes") && j.at("reveal_keyframes").is_array()) p.reveal_keyframes = j.at("reveal_keyframes").get<std::vector<ScalarKeyframeSpec>>();
}

void to_json(json& j, const TextAnimatorSpec& a) {
    j = json{{"selector", a.selector}, {"properties", a.properties}};
}

void from_json(const json& j, TextAnimatorSpec& a) {
    if (j.contains("selector") && j.at("selector").is_object()) {
        a.selector = j.at("selector").get<TextAnimatorSelectorSpec>();
    }
    if (j.contains("properties") && j.at("properties").is_object()) {
        a.properties = j.at("properties").get<TextAnimatorPropertySpec>();
    }
}

void to_json(json& j, const TextHighlightSpec& h) {
    j = json{
        {"start_glyph", h.start_glyph},
        {"end_glyph", h.end_glyph},
        {"color", h.color},
        {"padding_x", h.padding_x},
        {"padding_y", h.padding_y}
    };
}

void from_json(const json& j, TextHighlightSpec& h) {
    if (j.contains("start_glyph") && j.at("start_glyph").is_number_unsigned()) h.start_glyph = j.at("start_glyph").get<std::size_t>();
    if (j.contains("end_glyph") && j.at("end_glyph").is_number_unsigned()) h.end_glyph = j.at("end_glyph").get<std::size_t>();
    if (j.contains("color") && j.at("color").is_object()) h.color = j.at("color").get<ColorSpec>();
    if (j.contains("padding_x") && j.at("padding_x").is_number_integer()) h.padding_x = j.at("padding_x").get<std::int32_t>();
    if (j.contains("padding_y") && j.at("padding_y").is_number_integer()) h.padding_y = j.at("padding_y").get<std::int32_t>();
}

} // namespace tachyon
