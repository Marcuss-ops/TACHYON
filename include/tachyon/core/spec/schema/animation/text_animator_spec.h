#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

struct TextAnimatorSelectorSpec {
    std::string type{"range"}; // "range" | "all"
    double start{0.0};
    double end{1.0};
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
