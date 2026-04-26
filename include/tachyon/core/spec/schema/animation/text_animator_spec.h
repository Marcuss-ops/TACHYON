#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

struct TextAnimatorSelectorSpec {
    std::string type{"range"}; // "range" | "index" | "expression" | "all" | "random" | "wiggly"
    
    // For "range" (percentage 0.0-1.0)
    double start{0.0};
    double end{1.0};
    
    // For "index" (glyph/word indices)
    std::optional<std::size_t> start_index;
    std::optional<std::size_t> end_index;
    
    // For "expression"
    std::optional<std::string> expression;
    
    // For "random" / "wiggly"
    std::optional<std::uint64_t> seed;
    std::optional<double> amount;
    std::optional<double> frequency;
    bool random_order{false};

    // Selection mode (add, subtract, intersect)
    std::string mode{"add"};
    
    // Unit for selector evaluation:
    // - "characters": all glyphs including spaces
    // - "characters_excluding_spaces": only non-space glyphs
    // - "words": word-level selection (uses word_index)
    // - "lines": line-level selection (uses line_index)
    // - "clusters": grapheme cluster-level selection (preserves shaping)
    std::string based_on{"characters"};
    
    // Stagger mode for temporal offset:
    // - "none": no stagger (default)
    // - "character": stagger by character index
    // - "word": stagger by word index
    // - "line": stagger by line index
    std::string stagger_mode{"none"};
    
    // Delay in seconds per unit (character/word/line) for stagger effect
    double stagger_delay{0.0};
};

struct TextAnimatorPropertySpec {
    std::optional<double>            opacity_value;
    std::vector<ScalarKeyframeSpec>  opacity_keyframes;
    
    std::optional<math::Vector2>          position_offset_value;
    std::vector<Vector2KeyframeSpec>      position_offset_keyframes;
    
    std::optional<double>            scale_value;
    std::vector<ScalarKeyframeSpec>  scale_keyframes;
    
    std::optional<double>            rotation_value;
    std::vector<ScalarKeyframeSpec>  rotation_keyframes;
    
    // Advanced properties
    std::optional<double>            tracking_amount_value;
    std::vector<ScalarKeyframeSpec>  tracking_amount_keyframes;
    
    std::optional<ColorSpec>         fill_color_value;
    std::vector<ColorKeyframeSpec>   fill_color_keyframes;
    
    std::optional<ColorSpec>         stroke_color_value;
    std::vector<ColorKeyframeSpec>   stroke_color_keyframes;
    
    std::optional<double>            stroke_width_value;
    std::vector<ScalarKeyframeSpec>  stroke_width_keyframes;

    // Blur effect (gaussian blur radius in pixels)
    std::optional<double>            blur_radius_value;
    std::vector<ScalarKeyframeSpec>  blur_radius_keyframes;

    // Reveal effect (0.0 = fully hidden, 1.0 = fully revealed)
    // Applies a trim/reveal effect to glyphs based on coverage
    std::optional<double>            reveal_value;
    std::vector<ScalarKeyframeSpec>  reveal_keyframes;
};

struct TextAnimatorSpec {
    TextAnimatorSelectorSpec  selector;
    TextAnimatorPropertySpec  properties;
};

struct TextHighlightSpec {
    std::size_t start_glyph{0};
    std::size_t end_glyph{0}; // exclusive
    ColorSpec color{255, 236, 59, 96};
    std::int32_t padding_x{3};
    std::int32_t padding_y{2};
};

inline void to_json(nlohmann::json& j, const TextAnimatorSelectorSpec& s) {
    j = nlohmann::json{
        {"type", s.type},
        {"start", s.start},
        {"end", s.end},
        {"mode", s.mode},
        {"based_on", s.based_on},
        {"random_order", s.random_order},
        {"stagger_mode", s.stagger_mode},
        {"stagger_delay", s.stagger_delay}
    };
    if (s.start_index.has_value()) j["start_index"] = *s.start_index;
    if (s.end_index.has_value()) j["end_index"] = *s.end_index;
    if (s.expression.has_value()) j["expression"] = *s.expression;
    if (s.seed.has_value()) j["seed"] = *s.seed;
    if (s.amount.has_value()) j["amount"] = *s.amount;
    if (s.frequency.has_value()) j["frequency"] = *s.frequency;
}

inline void from_json(const nlohmann::json& j, TextAnimatorSelectorSpec& s) {
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
}

// JSON serialization declarations (implementations in text_animator_spec_serialize.cpp)
void to_json(nlohmann::json& j, const TextAnimatorPropertySpec& p);
void from_json(const nlohmann::json& j, TextAnimatorPropertySpec& p);
void to_json(nlohmann::json& j, const TextAnimatorSpec& a);
void from_json(const nlohmann::json& j, TextAnimatorSpec& a);
void to_json(nlohmann::json& j, const TextHighlightSpec& h);
void from_json(const nlohmann::json& j, TextHighlightSpec& h);

} // namespace tachyon
