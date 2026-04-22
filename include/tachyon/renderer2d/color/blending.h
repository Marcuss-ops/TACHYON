#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

enum class BlendMode {
    Normal,
    Dissolve,
    Darken,
    Multiply,
    ColorBurn,
    LinearBurn,
    DarkerColor,
    Lighten,
    Screen,
    ColorDodge,
    LinearDodge, // Add
    LighterColor,
    Overlay,
    SoftLight,
    HardLight,
    VividLight,
    LinearLight,
    PinLight,
    HardMix,
    Difference,
    Exclusion,
    Subtract,
    Divide,
    Hue,
    Saturation,
    Color,
    Luminosity,
    StencilAlpha,
    StencilLuma,
    SilhouetteAlpha,
    SilhouetteLuma,
    AlphaAdd,
    LuminescentPremult,
    // Additional modes for completeness
    Additive,
    ClassicColorBurn,
    ClassicColorDodge,
    ClassicDifference,
    DancingDissolve
};

namespace detail {

inline float lum(Color c) {
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

inline Color set_lum(Color c, float l) {
    float d = l - lum(c);
    c.r += d;
    c.g += d;
    c.b += d;
    float l2 = lum(c);
    float min_c = std::min({c.r, c.g, c.b});
    float max_c = std::max({c.r, c.g, c.b});
    if (min_c < 0.0f) {
        c.r = l2 + (((c.r - l2) * l2) / (l2 - min_c));
        c.g = l2 + (((c.g - l2) * l2) / (l2 - min_c));
        c.b = l2 + (((c.b - l2) * l2) / (l2 - min_c));
    }
    if (max_c > 1.0f) {
        c.r = l2 + (((c.r - l2) * (1.0f - l2)) / (max_c - l2));
        c.g = l2 + (((c.g - l2) * (1.0f - l2)) / (max_c - l2));
        c.b = l2 + (((c.b - l2) * (1.0f - l2)) / (max_c - l2));
    }
    return c;
}

inline float sat(Color c) {
    return std::max({c.r, c.g, c.b}) - std::min({c.r, c.g, c.b});
}

inline Color set_sat(Color c, float s) {
    float values[3] = {c.r, c.g, c.b};
    int indices[3] = {0, 1, 2};
    std::sort(indices, indices + 3, [&](int i, int j) { return values[i] < values[j]; });
    
    if (values[indices[2]] > values[indices[0]]) {
        float mid = values[indices[1]];
        float min = values[indices[0]];
        float max = values[indices[2]];
        
        float new_mid = ((mid - min) * s) / (max - min);
        float new_max = s;
        
        float res[3];
        res[indices[0]] = 0.0f;
        res[indices[1]] = new_mid;
        res[indices[2]] = new_max;
        return Color{res[0], res[1], res[2], c.a};
    } else {
        return Color{0.0f, 0.0f, 0.0f, c.a};
    }
}

} // namespace detail

inline Color blend_mode_color(Color src, Color dst, BlendMode mode) {
    // Normalizing colors to [0, 1] for blending math
    // Assuming input is already [0, 1] based on Color struct usage in other files
    
    Color res = src;
    
    switch (mode) {
        case BlendMode::Normal:
            return src;
            
        case BlendMode::Multiply:
            res.r = src.r * dst.r;
            res.g = src.g * dst.g;
            res.b = src.b * dst.b;
            break;
            
        case BlendMode::Screen:
            res.r = 1.0f - (1.0f - src.r) * (1.0f - dst.r);
            res.g = 1.0f - (1.0f - src.g) * (1.0f - dst.g);
            res.b = 1.0f - (1.0f - src.b) * (1.0f - dst.b);
            break;
            
        case BlendMode::Overlay:
            res.r = dst.r < 0.5f ? 2.0f * src.r * dst.r : 1.0f - 2.0f * (1.0f - src.r) * (1.0f - dst.r);
            res.g = dst.g < 0.5f ? 2.0f * src.g * dst.g : 1.0f - 2.0f * (1.0f - src.g) * (1.0f - dst.g);
            res.b = dst.b < 0.5f ? 2.0f * src.b * dst.b : 1.0f - 2.0f * (1.0f - src.b) * (1.0f - dst.b);
            break;
            
        case BlendMode::Darken:
            res.r = std::min(src.r, dst.r);
            res.g = std::min(src.g, dst.g);
            res.b = std::min(src.b, dst.b);
            break;
            
        case BlendMode::Lighten:
            res.r = std::max(src.r, dst.r);
            res.g = std::max(src.g, dst.g);
            res.b = std::max(src.b, dst.b);
            break;
            
        case BlendMode::ColorDodge:
            res.r = src.r == 1.0f ? 1.0f : std::min(1.0f, dst.r / (1.0f - src.r));
            res.g = src.g == 1.0f ? 1.0f : std::min(1.0f, dst.g / (1.0f - src.g));
            res.b = src.b == 1.0f ? 1.0f : std::min(1.0f, dst.b / (1.0f - src.b));
            break;
            
        case BlendMode::ColorBurn:
            res.r = src.r == 0.0f ? 0.0f : 1.0f - std::min(1.0f, (1.0f - dst.r) / src.r);
            res.g = src.g == 0.0f ? 0.0f : 1.0f - std::min(1.0f, (1.0f - dst.g) / src.g);
            res.b = src.b == 0.0f ? 0.0f : 1.0f - std::min(1.0f, (1.0f - dst.b) / src.b);
            break;
            
        case BlendMode::HardLight:
            res.r = src.r < 0.5f ? 2.0f * src.r * dst.r : 1.0f - 2.0f * (1.0f - src.r) * (1.0f - dst.r);
            res.g = src.g < 0.5f ? 2.0f * src.g * dst.g : 1.0f - 2.0f * (1.0f - src.g) * (1.0f - dst.g);
            res.b = src.b < 0.5f ? 2.0f * src.b * dst.b : 1.0f - 2.0f * (1.0f - src.b) * (1.0f - dst.b);
            break;
            
        case BlendMode::SoftLight:
            res.r = src.r < 0.5f ? dst.r - (1.0f - 2.0f * src.r) * dst.r * (1.0f - dst.r) : (dst.r < 0.25f ? dst.r + (2.0f * src.r - 1.0f) * dst.r * ((16.0f * dst.r - 12.0f) * dst.r + 3.0f) : dst.r + (2.0f * src.r - 1.0f) * (std::sqrt(dst.r) - dst.r));
            res.g = src.g < 0.5f ? dst.g - (1.0f - 2.0f * src.g) * dst.g * (1.0f - dst.g) : (dst.g < 0.25f ? dst.g + (2.0f * src.g - 1.0f) * dst.g * ((16.0f * dst.g - 12.0f) * dst.g + 3.0f) : dst.g + (2.0f * src.g - 1.0f) * (std::sqrt(dst.g) - dst.g));
            res.b = src.b < 0.5f ? dst.b - (1.0f - 2.0f * src.b) * dst.b * (1.0f - dst.b) : (dst.b < 0.25f ? dst.b + (2.0f * src.b - 1.0f) * dst.b * ((16.0f * dst.b - 12.0f) * dst.b + 3.0f) : dst.b + (2.0f * src.b - 1.0f) * (std::sqrt(dst.b) - dst.b));
            break;
            
        case BlendMode::Difference:
            res.r = std::abs(src.r - dst.r);
            res.g = std::abs(src.g - dst.g);
            res.b = std::abs(src.b - dst.b);
            break;
            
        case BlendMode::Exclusion:
            res.r = src.r + dst.r - 2.0f * src.r * dst.r;
            res.g = src.g + dst.g - 2.0f * src.g * dst.g;
            res.b = src.b + dst.b - 2.0f * src.b * dst.b;
            break;
            
        case BlendMode::Hue:
            res = detail::set_lum(detail::set_sat(src, detail::sat(dst)), detail::lum(dst));
            break;
            
        case BlendMode::Saturation:
            res = detail::set_lum(detail::set_sat(dst, detail::sat(src)), detail::lum(dst));
            break;
            
        case BlendMode::Color:
            res = detail::set_lum(src, detail::lum(dst));
            break;
            
        case BlendMode::Luminosity:
            res = detail::set_lum(dst, detail::lum(src));
            break;
            
        case BlendMode::Additive:
        case BlendMode::LinearDodge:
            res.r = std::min(1.0f, src.r + dst.r);
            res.g = std::min(1.0f, src.g + dst.g);
            res.b = std::min(1.0f, src.b + dst.b);
            break;
            
        case BlendMode::Subtract:
            res.r = std::max(0.0f, dst.r - src.r);
            res.g = std::max(0.0f, dst.g - src.g);
            res.b = std::max(0.0f, dst.b - src.b);
            break;
            
        case BlendMode::Divide:
            res.r = src.r == 0.0f ? 1.0f : std::min(1.0f, dst.r / src.r);
            res.g = src.g == 0.0f ? 1.0f : std::min(1.0f, dst.g / src.g);
            res.b = src.b == 0.0f ? 1.0f : std::min(1.0f, dst.b / src.b);
            break;

        case BlendMode::LinearBurn:
            res.r = std::max(0.0f, src.r + dst.r - 1.0f);
            res.g = std::max(0.0f, src.g + dst.g - 1.0f);
            res.b = std::max(0.0f, src.b + dst.b - 1.0f);
            break;

        case BlendMode::VividLight:
            {
                auto vlight = [](float s, float d) {
                    if (s < 0.5f) return s == 0.0f ? 0.0f : 1.0f - std::min(1.0f, (1.0f - d) / (2.0f * s));
                    return (2.0f * (s - 0.5f)) == 1.0f ? 1.0f : std::min(1.0f, d / (1.0f - 2.0f * (s - 0.5f)));
                };
                res.r = vlight(src.r, dst.r);
                res.g = vlight(src.g, dst.g);
                res.b = vlight(src.b, dst.b);
            }
            break;

        case BlendMode::LinearLight:
            res.r = std::clamp(dst.r + 2.0f * src.r - 1.0f, 0.0f, 1.0f);
            res.g = std::clamp(dst.g + 2.0f * src.g - 1.0f, 0.0f, 1.0f);
            res.b = std::clamp(dst.b + 2.0f * src.b - 1.0f, 0.0f, 1.0f);
            break;

        case BlendMode::PinLight:
            {
                auto plight = [](float s, float d) {
                    if (s < 0.5f) return std::min(d, 2.0f * s);
                    return std::max(d, 2.0f * (s - 0.5f));
                };
                res.r = plight(src.r, dst.r);
                res.g = plight(src.g, dst.g);
                res.b = plight(src.b, dst.b);
            }
            break;

        case BlendMode::HardMix:
            res.r = (src.r + dst.r >= 1.0f) ? 1.0f : 0.0f;
            res.g = (src.g + dst.g >= 1.0f) ? 1.0f : 0.0f;
            res.b = (src.b + dst.b >= 1.0f) ? 1.0f : 0.0f;
            break;

        case BlendMode::DarkerColor:
            return (detail::lum(src) < detail::lum(dst)) ? src : dst;

        case BlendMode::LighterColor:
            return (detail::lum(src) > detail::lum(dst)) ? src : dst;
            
        default:
            break;
    }
    
    // Alpha compositing: result alpha is src alpha (simplified for layer blend)
    // In AE, layer blending usually keeps the src alpha for the layer's influence.
    // However, the standard formula is more complex. 
    // For now, we return the blended color with src alpha.
    res.a = src.a;
    return res;
}

BlendMode parse_blend_mode(const std::string& name);

} // namespace tachyon::renderer2d
