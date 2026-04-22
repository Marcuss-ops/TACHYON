#pragma once

#include "tachyon/renderer2d/color/color_profile.h"
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>

namespace tachyon::renderer2d {

using ColorSpace = ColorPrimaries;

enum class ColorRange {
    Full,
    Limited
};

using TransferCurve = ::tachyon::renderer2d::TransferCurve;

inline ColorSpace parse_color_space(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

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
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (lower == "tv" || lower == "limited" || lower == "legal") return ColorRange::Limited;
    return ColorRange::Full;
}

inline TransferCurve parse_transfer_curve(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (lower == "linear" || lower == "linear_rec709" || lower == "rec709_linear") return TransferCurve::Linear;
    if (lower == "bt709" || lower == "rec709" || lower == "gamma_2.4") return TransferCurve::Rec709;
    return TransferCurve::sRGB;
}

} // namespace tachyon::renderer2d
