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

namespace detail {
    using ColorPrimaries = ::tachyon::renderer2d::ColorPrimaries;
    using ColorSpace = ::tachyon::renderer2d::ColorSpace;
    using TransferCurve = ::tachyon::renderer2d::TransferCurve;
    using ColorRange = ::tachyon::renderer2d::ColorRange;
    using ::tachyon::renderer2d::apply_range_mode;
    using ::tachyon::renderer2d::composite_src_over_linear;
    using ::tachyon::renderer2d::convert_color;
    using ::tachyon::renderer2d::primaries_conversion_matrix;
    using ::tachyon::renderer2d::linear_to_transfer_component;
    using ::tachyon::renderer2d::srgb_to_linear_component;
    using ::tachyon::renderer2d::parse_color_space;
    using ::tachyon::renderer2d::parse_color_primaries;
    using ::tachyon::renderer2d::parse_color_range;
    using ::tachyon::renderer2d::parse_transfer_curve;
    using ::tachyon::renderer2d::sRGB_to_Linear_f;
    using ::tachyon::renderer2d::Linear_to_sRGB_f;
    using ::tachyon::renderer2d::linear_to_srgb_component;
}

// Forward declarations or includes for missing blend functions
BlendMode parse_blend_mode(const std::string& name);
Color blend_mode_color(Color src, Color dest, BlendMode mode);

} // namespace tachyon::renderer2d
