#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tachyon::renderer2d {

enum class ColorPrimaries {
    sRGB,       // Same as Rec.709
    Rec709 = sRGB,
    Rec2020,
    P3D65,      // Display P3
    P3DCI,      // DCI-P3 (theatrical)
    ACES_AP0,   // ACES Primaries 0 (for archival/storage)
    ACES_AP1,   // ACES Primaries 1 (for working space ACEScg/ACEScc)
    Custom
};

enum class TransferCurve {
    Linear,
    sRGB,
    Rec709,     // Bt.709 / Bt.1886 (Gamma 2.4)
    Gamma22,
    Gamma24,
    Gamma26,
    PQ,         // ST 2084 (HDR10)
    HLG,        // ARIB STD-B67
    LogC3,      // ARRI Log C
    LogC4,      // ARRI Log C4
    SLog3,      // Sony S-Log3
    VLog,       // Panasonic V-Log
    Custom
};

enum class WhitePoint {
    D65,
    D60,        // ACES
    D50,
    DCI,        // Theater
    Custom
};

struct ColorProfile {
    ColorPrimaries primaries{ColorPrimaries::sRGB};
    TransferCurve curve{TransferCurve::sRGB};
    WhitePoint white_point{WhitePoint::D65};

    static ColorProfile sRGB() {
        return {ColorPrimaries::sRGB, TransferCurve::sRGB, WhitePoint::D65};
    }

    static ColorProfile Rec709() {
        return {ColorPrimaries::Rec709, TransferCurve::Rec709, WhitePoint::D65};
    }

    static ColorProfile Rec2020_Linear() {
        return {ColorPrimaries::Rec2020, TransferCurve::Linear, WhitePoint::D65};
    }

    static ColorProfile ACEScg() {
        return {ColorPrimaries::ACES_AP1, TransferCurve::Linear, WhitePoint::D60};
    }

    bool operator==(const ColorProfile& other) const {
        return primaries == other.primaries && curve == other.curve && white_point == other.white_point;
    }

    bool operator!=(const ColorProfile& other) const {
        return !(*this == other);
    }
};

struct WorkingColorSpace {
    ColorProfile profile{ColorProfile::ACEScg()};
    bool linear{true};
};

} // namespace tachyon::renderer2d
