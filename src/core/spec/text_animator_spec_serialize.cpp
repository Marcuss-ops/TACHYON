#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include <nlohmann/json.hpp>
#include "tachyon/core/spec/property_spec_serialize_helpers.h"

using json = nlohmann::json;

namespace tachyon {

void to_json(json& j, const TextAnimatorSelectorSpec& s) {
    j = json{
        {"type", s.type},
        {"start", s.start},
        {"end", s.end},
        {"mode", s.mode},
        {"based_on", s.based_on},
        {"random_order", s.random_order},
        {"stagger_mode", s.stagger_mode},
        {"stagger_delay", s.stagger_delay},
        {"shape", s.shape},
        {"offset", s.offset},
        {"ease_high", s.ease_high},
        {"ease_low", s.ease_low}
    };
    if (s.start_index.has_value()) j["start_index"] = *s.start_index;
    if (s.end_index.has_value()) j["end_index"] = *s.end_index;
    if (s.expression.has_value()) j["expression"] = *s.expression;
    if (s.seed.has_value()) j["seed"] = *s.seed;
    if (s.amount.has_value()) j["amount"] = *s.amount;
    if (s.frequency.has_value()) j["frequency"] = *s.frequency;
}

void from_json(const json& j, TextAnimatorSelectorSpec& s) {
    if (j.contains("type") && j.at("type").is_string()) s.type = j.at("type").get<std::string>();
    if (j.contains("start") && j.at("start").is_number()) s.start = j.at("start").get<double>();
    if (j.contains("end") && j.at("end").is_number()) s.end = j.at("end").get<double>();
    if (j.contains("start_index") && j.at("start_index").is_number_unsigned()) s.start_index = j.at("start_index").get<std::size_t>();
    if (j.contains("end_index") && j.at("end_index").is_number_unsigned()) s.end_index = j.at("end_index").get<std::size_t>();
    if (j.contains("expression") && j.at("expression").is_string()) s.expression = j.at("expression").get<std::string>();
    if (j.contains("seed") && j.at("seed").is_number_unsigned()) s.seed = j.at("seed").get<std::uint64_t>();
    if (j.contains("amount") && j.at("amount").is_number()) s.amount = j.at("amount").get<double>();
    if (j.contains("frequency") && j.at("frequency").is_number()) s.frequency = j.at("frequency").get<double>();
    if (j.contains("random_order") && j.at("random_order").is_boolean()) s.random_order = j.at("random_order").get<bool>();
    if (j.contains("mode") && j.at("mode").is_string()) s.mode = j.at("mode").get<std::string>();
    if (j.contains("based_on") && j.at("based_on").is_string()) s.based_on = j.at("based_on").get<std::string>();
    if (j.contains("stagger_mode") && j.at("stagger_mode").is_string()) s.stagger_mode = j.at("stagger_mode").get<std::string>();
    if (j.contains("stagger_delay") && j.at("stagger_delay").is_number()) s.stagger_delay = j.at("stagger_delay").get<double>();
    if (j.contains("shape") && j.at("shape").is_string()) s.shape = j.at("shape").get<std::string>();
    if (j.contains("offset") && j.at("offset").is_number()) s.offset = j.at("offset").get<double>();
    if (j.contains("ease_high") && j.at("ease_high").is_number()) s.ease_high = j.at("ease_high").get<double>();
    if (j.contains("ease_low") && j.at("ease_low").is_number()) s.ease_low = j.at("ease_low").get<double>();
}

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

void to_json(json& j, const TextAnimatorCursorSpec& c) {
    j = json{
        {"enabled", c.enabled},
        {"cursor_char", c.cursor_char},
        {"blink_rate", c.blink_rate},
        {"follow_last_glyph", c.follow_last_glyph},
        {"offset_x", c.offset_x}
    };
    if (c.color_override.has_value()) {
        j["color_override"] = *c.color_override;
    }
}

void from_json(const json& j, TextAnimatorCursorSpec& c) {
    if (j.contains("enabled") && j.at("enabled").is_boolean()) c.enabled = j.at("enabled").get<bool>();
    if (j.contains("cursor_char") && j.at("cursor_char").is_string()) c.cursor_char = j.at("cursor_char").get<std::string>();
    if (j.contains("blink_rate") && j.at("blink_rate").is_number()) c.blink_rate = j.at("blink_rate").get<double>();
    if (j.contains("color_override") && j.at("color_override").is_object()) c.color_override = j.at("color_override").get<ColorSpec>();
    if (j.contains("follow_last_glyph") && j.at("follow_last_glyph").is_boolean()) c.follow_last_glyph = j.at("follow_last_glyph").get<bool>();
    if (j.contains("offset_x") && j.at("offset_x").is_number()) c.offset_x = j.at("offset_x").get<double>();
}

void to_json(json& j, const TextAnimatorSpec& a) {
    j = json{
        {"selector", a.selector},
        {"properties", a.properties},
        {"cursor", a.cursor}
    };
    if (!a.name.empty()) {
        j["name"] = a.name;
    }
}

void from_json(const json& j, TextAnimatorSpec& a) {
    if (j.contains("name") && j.at("name").is_string()) {
        a.name = j.at("name").get<std::string>();
    }
    if (j.contains("selector") && j.at("selector").is_object()) {
        a.selector = j.at("selector").get<TextAnimatorSelectorSpec>();
    }
    if (j.contains("properties") && j.at("properties").is_object()) {
        a.properties = j.at("properties").get<TextAnimatorPropertySpec>();
    }
    if (j.contains("cursor") && j.at("cursor").is_object()) {
        a.cursor = j.at("cursor").get<TextAnimatorCursorSpec>();
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
