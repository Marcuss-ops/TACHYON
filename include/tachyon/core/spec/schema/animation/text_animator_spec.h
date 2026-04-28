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

    // AE-style Advanced Selector properties
    std::string shape{"square"}; // "square" | "ramp_up" | "ramp_down" | "triangle" | "round" | "smooth"
    double offset{0.0};          // Offset the range in percent
    double ease_high{0.0};       // S-curve ease in percent (0-100)
    double ease_low{0.0};        // S-curve ease in percent (0-100)
    
    // Selection mode (add, subtract, intersect)
    std::string mode{"add"};
    
    // Unit for selector evaluation:
    // ... (rest of the struct)
    std::string based_on{"characters"};
    
    std::string stagger_mode{"none"};
    
    double stagger_delay{0.0};
};

struct TextAnimatorCursorSpec {
    bool enabled{false};
    std::string cursor_char{"|"};
    double blink_rate{2.0}; // Hz (blinks per second)
    std::optional<ColorSpec> color_override;
    bool follow_last_glyph{true};
    double offset_x{2.0};
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
    std::string name;
    TextAnimatorSelectorSpec  selector;
    TextAnimatorPropertySpec  properties;
    TextAnimatorCursorSpec    cursor;
};

struct TextHighlightSpec {
    std::size_t start_glyph{0};
    std::size_t end_glyph{0}; // exclusive
    ColorSpec color{255, 236, 59, 96};
    std::int32_t padding_x{3};
    std::int32_t padding_y{2};
};

// JSON serialization declarations (implementations in text_animator_spec_serialize.cpp)
void to_json(nlohmann::json& j, const TextAnimatorSelectorSpec& s);
void from_json(const nlohmann::json& j, TextAnimatorSelectorSpec& s);
void to_json(nlohmann::json& j, const TextAnimatorPropertySpec& p);
void from_json(const nlohmann::json& j, TextAnimatorPropertySpec& p);
void to_json(nlohmann::json& j, const TextAnimatorSpec& a);
void from_json(const nlohmann::json& j, TextAnimatorSpec& a);
void to_json(nlohmann::json& j, const TextHighlightSpec& h);
void from_json(const nlohmann::json& j, TextHighlightSpec& h);

} // namespace tachyon
