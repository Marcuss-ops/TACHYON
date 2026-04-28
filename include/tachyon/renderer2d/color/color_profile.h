#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tachyon::renderer2d {

enum class ColorPrimaries {
    sRGB,       // Same as Rec.709
    Rec709,
    Rec2020,
    DisplayP3,  // Display P3
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

    static ColorProfile DisplayP3() {
        return {ColorPrimaries::DisplayP3, TransferCurve::Linear, WhitePoint::D65};
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

    std::string to_string() const {
        switch (primaries) {
            case ColorPrimaries::sRGB:
            case ColorPrimaries::Rec709:
                if (curve == TransferCurve::Linear) return "linear_rec709";
                if (curve == TransferCurve::sRGB) return "srgb";
                if (curve == TransferCurve::Rec709) return "rec709";
                break;
            case ColorPrimaries::Rec2020:
                if (curve == TransferCurve::Linear) return "linear_rec2020";
                if (curve == TransferCurve::PQ) return "pq_rec2020";
                if (curve == TransferCurve::HLG) return "hlg_rec2020";
                break;
            case ColorPrimaries::DisplayP3:
            case ColorPrimaries::P3D65:
                if (curve == TransferCurve::Linear) return "linear_displayp3";
                if (curve == TransferCurve::sRGB) return "srgb_displayp3";
                break;
            case ColorPrimaries::P3DCI:
                return "dci_p3";
            case ColorPrimaries::ACES_AP0:
                return "aces_ap0";
            case ColorPrimaries::ACES_AP1:
                if (curve == TransferCurve::Linear) return "acescg";
                break;
            default:
                break;
        }
        return "linear_rec709"; // safe fallback
    }
};

struct WorkingColorSpace {
    ColorProfile profile{ColorProfile::ACEScg()};
    bool linear{true};
};

} // namespace tachyon::renderer2d
