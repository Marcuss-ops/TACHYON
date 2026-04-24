#pragma once

#include "tachyon/renderer2d/color/color_profile.h"
#include "tachyon/renderer2d/color/color_matrix.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

struct Chromaticity {
    float x, y;
};

struct PrimarySet {
    Chromaticity r, g, b, w;
};

inline PrimarySet get_primary_set(ColorPrimaries primaries, WhitePoint white_point) {
    PrimarySet ps;
    
    // Set primaries
    switch (primaries) {
        case ColorPrimaries::Rec2020:
            ps.r = {0.708f, 0.292f}; ps.g = {0.170f, 0.797f}; ps.b = {0.131f, 0.046f};
            break;
        case ColorPrimaries::DisplayP3:
        case ColorPrimaries::P3D65:
        case ColorPrimaries::P3DCI:
            ps.r = {0.680f, 0.320f}; ps.g = {0.265f, 0.690f}; ps.b = {0.150f, 0.060f};
            break;
        case ColorPrimaries::ACES_AP0:
            ps.r = {0.7347f, 0.2653f}; ps.g = {0.0000f, 1.0000f}; ps.b = {0.0001f, -0.0770f};
            break;
        case ColorPrimaries::ACES_AP1:
            ps.r = {0.713f, 0.300f}; ps.g = {0.165f, 0.830f}; ps.b = {0.128f, 0.044f};
            break;
        case ColorPrimaries::sRGB:
        default:
            ps.r = {0.640f, 0.330f}; ps.g = {0.300f, 0.600f}; ps.b = {0.150f, 0.060f};
            break;
    }

    // Set white point
    switch (white_point) {
        case WhitePoint::D60: // ACES
            ps.w = {0.32168f, 0.33767f};
            break;
        case WhitePoint::D50:
            ps.w = {0.3457f, 0.3585f};
            break;
        case WhitePoint::DCI:
            ps.w = {0.314f, 0.351f};
            break;
        case WhitePoint::D65:
        default:
            ps.w = {0.3127f, 0.3290f};
            break;
    }

    return ps;
}

// Generates a matrix to convert from RGB to XYZ for a given primary set
inline Matrix3x3 calculate_rgb_to_xyz(const PrimarySet& ps) {
    auto x_y_to_X_Y_Z = [](float x, float y) {
        return std::array<float, 3>{x / y, 1.0f, (1.0f - x - y) / y};
    };

    auto r = x_y_to_X_Y_Z(ps.r.x, ps.r.y);
    auto g = x_y_to_X_Y_Z(ps.g.x, ps.g.y);
    auto b = x_y_to_X_Y_Z(ps.b.x, ps.b.y);
    auto w = x_y_to_X_Y_Z(ps.w.x, ps.w.y);

    Matrix3x3 M{{r[0], g[0], b[0], r[1], g[1], b[1], r[2], g[2], b[2]}};
    Matrix3x3 invM = invert(M);

    float sr = invM.m[0] * w[0] + invM.m[1] * w[1] + invM.m[2] * w[2];
    float sg = invM.m[3] * w[0] + invM.m[4] * w[1] + invM.m[5] * w[2];
    float sb = invM.m[6] * w[0] + invM.m[7] * w[1] + invM.m[8] * w[2];

    return Matrix3x3{{sr * r[0], sg * g[0], sb * b[0], sr * r[1], sg * g[1], sb * b[1], sr * r[2], sg * g[2], sb * b[2]}};
}

// Transfer Functions (EOTF and OETF)
struct TransferFunctions {
    static float to_linear(float v, TransferCurve curve) {
        switch (curve) {
            case TransferCurve::sRGB:
                return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
            case TransferCurve::Rec709:
                return v < 0.081f ? v / 4.5f : std::pow((v + 0.099f) / 1.099f, 1.0f / 0.45f);
            case TransferCurve::Gamma22: return std::pow(std::max(0.0f, v), 2.2f);
            case TransferCurve::Gamma24: return std::pow(std::max(0.0f, v), 2.4f);
            case TransferCurve::Gamma26: return std::pow(std::max(0.0f, v), 2.6f);
            case TransferCurve::PQ: {
                float v_pow = std::pow(std::max(0.0f, v), 1.0f / 78.84375f);
                float num = std::max(v_pow - 0.8359375f, 0.0f);
                float den = 18.8515625f - 18.6875f * v_pow;
                return 10000.0f * std::pow(num / den, 1.0f / 14.359375f); // Result in nits, usually normalized later
            }
            case TransferCurve::SLog3:
                return v >= 0.01125f ? std::pow(10.0f, (v - 0.170f) / 0.42125f) * (0.18f + 0.01f) - 0.01f : (v - 0.092809f) * 0.18f / (0.171541f - 0.092809f);
            case TransferCurve::Linear:
            default: return v;
        }
    }

    static float from_linear(float v, TransferCurve curve) {
        switch (curve) {
            case TransferCurve::sRGB:
                return v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
            case TransferCurve::Rec709:
                return v < 0.018f ? v * 4.5f : 1.099f * std::pow(v, 0.45f) - 0.099f;
            case TransferCurve::Gamma22: return std::pow(std::max(0.0f, v), 1.0f / 2.2f);
            case TransferCurve::Gamma24: return std::pow(std::max(0.0f, v), 1.0f / 2.4f);
            case TransferCurve::Gamma26: return std::pow(std::max(0.0f, v), 1.0f / 2.6f);
            case TransferCurve::PQ: {
                float l = std::pow(std::max(0.0f, v / 10000.0f), 0.1593017578125f);
                return std::pow((0.8359375f + 18.8515625f * l) / (1.0f + 18.6875f * l), 78.84375f);
            }
            case TransferCurve::SLog3:
                return v > 0.01125f ? 0.42125f * std::log10((v + 0.01f) / (0.18f + 0.01f)) + 0.170f : v * (0.171541f - 0.092809f) / 0.18f + 0.092809f;
            case TransferCurve::Linear:
            default: return v;
        }
    }
};

} // namespace tachyon::renderer2d
