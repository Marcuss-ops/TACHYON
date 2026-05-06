#include "tachyon/color/blending.h"
#include "tachyon/color/color_math.h"
#include <algorithm>
#include <cmath>

namespace tachyon::color {

LinearRGBA blend_normal(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    if (out_a <= 0.0f) return LinearRGBA{0,0,0,0};
    
    return LinearRGBA{
        (src.r * src.a + dst.r * dst.a * (1.0f - src.a)) / out_a,
        (src.g * src.a + dst.g * dst.a * (1.0f - src.a)) / out_a,
        (src.b * src.a + dst.b * dst.a * (1.0f - src.a)) / out_a,
        out_a
    };
}

LinearRGBA blend_multiply(LinearRGBA src, LinearRGBA dst) {
    return LinearRGBA{ src.r * dst.r, src.g * dst.g, src.b * dst.b, src.a * dst.a };
}

LinearRGBA blend_screen(LinearRGBA src, LinearRGBA dst) {
    return LinearRGBA{
        src.r + dst.r - src.r * dst.r,
        src.g + dst.g - src.g * dst.g,
        src.b + dst.b - src.b * dst.b,
        src.a + dst.a - src.a * dst.a
    };
}

LinearRGBA blend_overlay(LinearRGBA src, LinearRGBA dst) {
    auto overlay = [](float s, float d) {
        return d < 0.5f ? 2.0f * s * d : 1.0f - 2.0f * (1.0f - s) * (1.0f - d);
    };
    float out_a = src.a + dst.a * (1.0f - src.a);
    if (out_a <= 0.0f) return LinearRGBA{0, 0, 0, 0};
    return LinearRGBA{ overlay(src.r, dst.r), overlay(src.g, dst.g), overlay(src.b, dst.b), out_a };
}

LinearRGBA blend_additive(LinearRGBA src, LinearRGBA dst) {
    return LinearRGBA{
        std::min(1.0f, src.r + dst.r), std::min(1.0f, src.g + dst.g), std::min(1.0f, src.b + dst.b), std::min(1.0f, src.a + dst.a)
    };
}

LinearRGBA blend_soft_light(LinearRGBA src, LinearRGBA dst) {
    auto soft_light = [](float s, float d) -> float {
        if (s <= 0.5f) return d - (1.0f - 2.0f * s) * d * (1.0f - d);
        return d + (2.0f * s - 1.0f) * (std::sqrt(d) - d);
    };
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ soft_light(src.r, dst.r), soft_light(src.g, dst.g), soft_light(src.b, dst.b), out_a };
}

LinearRGBA blend_darken(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ std::min(src.r, dst.r), std::min(src.g, dst.g), std::min(src.b, dst.b), out_a };
}

LinearRGBA blend_lighten(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ std::max(src.r, dst.r), std::max(src.g, dst.g), std::max(src.b, dst.b), out_a };
}

LinearRGBA blend_color_dodge(LinearRGBA src, LinearRGBA dst) {
    auto dodge = [](float s, float d) {
        if (d <= 0.0f) return 0.0f;
        if (s >= 1.0f) return 1.0f;
        return std::min(1.0f, d / (1.0f - s));
    };
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ dodge(src.r, dst.r), dodge(src.g, dst.g), dodge(src.b, dst.b), out_a };
}

LinearRGBA blend_color_burn(LinearRGBA src, LinearRGBA dst) {
    auto burn = [](float s, float d) {
        if (d >= 1.0f) return 1.0f;
        if (s <= 0.0f) return 0.0f;
        return 1.0f - std::min(1.0f, (1.0f - d) / s);
    };
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ burn(src.r, dst.r), burn(src.g, dst.g), burn(src.b, dst.b), out_a };
}

LinearRGBA blend_hard_light(LinearRGBA src, LinearRGBA dst) {
    return blend_overlay(dst, src);
}

LinearRGBA blend_difference(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ std::abs(src.r - dst.r), std::abs(src.g - dst.g), std::abs(src.b - dst.b), out_a };
}

LinearRGBA blend_exclusion(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{
        src.r + dst.r - 2.0f * src.r * dst.r,
        src.g + dst.g - 2.0f * src.g * dst.g,
        src.b + dst.b - 2.0f * src.b * dst.b,
        out_a
    };
}

LinearRGBA blend_linear_dodge(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ std::min(1.0f, src.r + dst.r), std::min(1.0f, src.g + dst.g), std::min(1.0f, src.b + dst.b), out_a };
}

LinearRGBA blend_subtract(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ std::max(0.0f, dst.r - src.r), std::max(0.0f, dst.g - src.g), std::max(0.0f, dst.b - src.b), out_a };
}

LinearRGBA blend_divide(LinearRGBA src, LinearRGBA dst) {
    auto divide = [](float s, float d) { return d > 0.0f ? std::min(1.0f, s / d) : 1.0f; };
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ divide(src.r, dst.r), divide(src.g, dst.g), divide(src.b, dst.b), out_a };
}

LinearRGBA blend_linear_burn(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{
        std::max(0.0f, src.r + dst.r - 1.0f), std::max(0.0f, src.g + dst.g - 1.0f), std::max(0.0f, src.b + dst.b - 1.0f), out_a
    };
}

LinearRGBA blend_vivid_light(LinearRGBA src, LinearRGBA dst) {
    auto vivid = [](float s, float d) {
        if (s <= 0.5f) {
            if (s <= 0.0f) return 0.0f;
            return 1.0f - std::min(1.0f, (1.0f - d) / (2.0f * s));
        }
        if (s >= 1.0f) return 1.0f;
        return std::min(1.0f, d / (2.0f * (1.0f - s)));
    };
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ vivid(src.r, dst.r), vivid(src.g, dst.g), vivid(src.b, dst.b), out_a };
}

LinearRGBA blend_linear_light(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{
        std::clamp(dst.r + 2.0f * src.r - 1.0f, 0.0f, 1.0f),
        std::clamp(dst.g + 2.0f * src.g - 1.0f, 0.0f, 1.0f),
        std::clamp(dst.b + 2.0f * src.b - 1.0f, 0.0f, 1.0f),
        out_a
    };
}

LinearRGBA blend_pin_light(LinearRGBA src, LinearRGBA dst) {
    auto pin = [](float s, float d) {
        if (s <= 0.5f) return std::min(d, 2.0f * s);
        return std::max(d, 2.0f * (s - 0.5f));
    };
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{ pin(src.r, dst.r), pin(src.g, dst.g), pin(src.b, dst.b), out_a };
}

LinearRGBA blend_hard_mix(LinearRGBA src, LinearRGBA dst) {
    float out_a = src.a + dst.a * (1.0f - src.a);
    return LinearRGBA{
        (src.r + dst.r) >= 1.0f ? 1.0f : 0.0f,
        (src.g + dst.g) >= 1.0f ? 1.0f : 0.0f,
        (src.b + dst.b) >= 1.0f ? 1.0f : 0.0f,
        out_a
    };
}

LinearRGBA blend_hue(LinearRGBA src, LinearRGBA dst) {
    float sh, ss, sl, dh, ds, dl;
    rgb_to_hsl(src.r, src.g, src.b, sh, ss, sl);
    rgb_to_hsl(dst.r, dst.g, dst.b, dh, ds, dl);
    return hsl_to_rgb(sh, ds, dl, src.a + dst.a * (1.0f - src.a));
}

LinearRGBA blend_saturation(LinearRGBA src, LinearRGBA dst) {
    float sh, ss, sl, dh, ds, dl;
    rgb_to_hsl(src.r, src.g, src.b, sh, ss, sl);
    rgb_to_hsl(dst.r, dst.g, dst.b, dh, ds, dl);
    return hsl_to_rgb(dh, ss, dl, src.a + dst.a * (1.0f - src.a));
}

LinearRGBA blend_color(LinearRGBA src, LinearRGBA dst) {
    float sh, ss, sl, dh, ds, dl;
    rgb_to_hsl(src.r, src.g, src.b, sh, ss, sl);
    rgb_to_hsl(dst.r, dst.g, dst.b, dh, ds, dl);
    return hsl_to_rgb(sh, ss, dl, src.a + dst.a * (1.0f - src.a));
}

LinearRGBA blend_luminosity(LinearRGBA src, LinearRGBA dst) {
    float sh, ss, sl, dh, ds, dl;
    rgb_to_hsl(src.r, src.g, src.b, sh, ss, sl);
    rgb_to_hsl(dst.r, dst.g, dst.b, dh, ds, dl);
    return hsl_to_rgb(dh, ds, sl, src.a + dst.a * (1.0f - src.a));
}

LinearRGBA blend_darker_color(LinearRGBA src, LinearRGBA dst) {
    float src_sum = src.r + src.g + src.b;
    float dst_sum = dst.r + dst.g + dst.b;
    float out_a = src.a + dst.a * (1.0f - src.a);
    if (src_sum <= dst_sum) return LinearRGBA{ src.r, src.g, src.b, out_a };
    return LinearRGBA{ dst.r, dst.g, dst.b, out_a };
}

LinearRGBA blend_lighter_color(LinearRGBA src, LinearRGBA dst) {
    float src_sum = src.r + src.g + src.b;
    float dst_sum = dst.r + dst.g + dst.b;
    float out_a = src.a + dst.a * (1.0f - src.a);
    if (src_sum >= dst_sum) return LinearRGBA{ src.r, src.g, src.b, out_a };
    return LinearRGBA{ dst.r, dst.g, dst.b, out_a };
}

} // namespace tachyon::color
