#pragma once

#include "tachyon/renderer2d/color/color_space.h"
#include "tachyon/renderer2d/color/transfer_functions.h"
#include "tachyon/renderer2d/color/color_matrix.h"
#include "tachyon/renderer2d/color/blending.h"

namespace tachyon::renderer2d {

// Compatibility helpers
inline Color apply_range_mode(Color color, ColorRange range) {
    if (range == ColorRange::Full) return color;
    auto scale = [](float value) -> float { return (16.0f + value * 219.0f) / 255.0f; };
    return Color{scale(color.r), scale(color.g), scale(color.b), color.a};
}

inline std::string_view ascii_lower(std::string_view value) {
    return value;
}

namespace detail {
    using namespace tachyon::renderer2d;
    using ::tachyon::renderer2d::sRGB_to_Linear_f;
    using ::tachyon::renderer2d::Linear_to_sRGB_f;
    using ::tachyon::renderer2d::linear_to_srgb_component;
    using ::tachyon::renderer2d::ascii_lower;
}

// Forward declarations or includes for missing blend functions
BlendMode parse_blend_mode(const std::string& name);
Color blend_mode_color(Color src, Color dest, BlendMode mode);

} // namespace tachyon::renderer2d
