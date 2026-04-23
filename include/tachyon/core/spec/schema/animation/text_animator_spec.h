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
    
    // Unit (characters, characters excluding spaces, words, lines)
    std::string based_on{"characters"};
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

} // namespace tachyon
