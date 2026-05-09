#pragma once

#include <cstdint>

namespace tachyon {

enum class HorizontalAlign {
    Left,
    Center,
    Right,
    Justify
};

enum class VerticalAlign {
    Top,
    Middle,
    Bottom
};

enum class TextBoxMode {
    Auto,
    Fixed
};

struct TextBoxSpec {
    TextBoxMode mode = TextBoxMode::Auto;
    float width = 0.0f;
    float height = 0.0f;
    HorizontalAlign horizontal_align = HorizontalAlign::Left;
    VerticalAlign vertical_align = VerticalAlign::Top;
    float line_height_factor = 1.0f;
    float tracking_amount = 0.0f;
    bool fixed_pitch = false;
};

} // namespace tachyon
