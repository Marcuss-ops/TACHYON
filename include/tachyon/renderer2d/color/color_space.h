#pragma once

#include "tachyon/core/string_utils.h"
#include "tachyon/renderer2d/color/color_profile.h"
#include <string>
#include <string_view>

namespace tachyon::renderer2d {

using ColorSpace = ColorPrimaries;

enum class ColorRange {
    Full,
    Limited
};

using TransferCurve = ::tachyon::renderer2d::TransferCurve;

inline ColorSpace parse_color_space(std::string_view value) {
    const std::string lower = tachyon::ascii_lower(value);

    if (lower == "bt709" || lower == "rec709") return ColorPrimaries::Rec709;
    if (lower == "display-p3") return ColorPrimaries::DisplayP3;
    if (lower == "p3" || lower == "dci-p3") return ColorPrimaries::P3D65;
    if (lower == "linear") return ColorPrimaries::sRGB;
    return ColorPrimaries::sRGB;
}

inline ColorPrimaries parse_color_primaries(std::string_view value) {
    return parse_color_space(value);
}

inline ColorRange parse_color_range(std::string_view value) {
    const std::string lower = tachyon::ascii_lower(value);

    if (lower == "tv" || lower == "limited" || lower == "legal") return ColorRange::Limited;
    return ColorRange::Full;
}

inline TransferCurve parse_transfer_curve(std::string_view value) {
    const std::string lower = tachyon::ascii_lower(value);

    if (lower == "linear" || lower == "linear_rec709" || lower == "rec709_linear") return TransferCurve::Linear;
    if (lower == "bt709" || lower == "rec709" || lower == "gamma_2.4") return TransferCurve::Rec709;
    return TransferCurve::sRGB;
}

} // namespace tachyon::renderer2d
