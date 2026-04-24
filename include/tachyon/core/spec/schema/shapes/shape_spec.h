#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

struct ShapeSpec {
    std::string type;  // "rectangle", "rounded_rect", "circle", "ellipse",
                       // "line", "arrow", "polygon", "star",
                       // "speech_bubble", "callout", "badge",
                       // "path"

    // Dimensions / position
    float x{0}, y{0};
    float width{0}, height{0};
    float radius{0};

    // Polygon / star
    int sides{6};
    float inner_radius{0};
    float outer_radius{0};

    // Arrow / line
    float x1{0}, y1{0};
    float head_size{10.0f};

    // Speech bubble / callout
    float tail_x{0}, tail_y{0};

    // Stroke
    float stroke_width{0};
    std::string line_cap{"butt"};
    std::string line_join{"miter"};
    std::vector<float> dash_array;
    float dash_offset{0};

    // Fill
    ColorSpec fill_color{255, 255, 255, 255};
    std::optional<GradientSpec> gradient_fill;

    // Stroke color
    ColorSpec stroke_color{0, 0, 0, 255};
    std::optional<GradientSpec> gradient_stroke;

    float opacity{1.0f};
};

} // namespace tachyon