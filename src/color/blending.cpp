#include "tachyon/color/blending.h"
#include <algorithm>

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
    return LinearRGBA{
        src.r * dst.r,
        src.g * dst.g,
        src.b * dst.b,
        src.a * dst.a
    };
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
    auto overlay_channel = [](float s, float d) {
        if (d < 0.5f) return 2.0f * s * d;
        return 1.0f - 2.0f * (1.0f - s) * (1.0f - d);
    };
    return LinearRGBA{
        overlay_channel(src.r, dst.r),
        overlay_channel(src.g, dst.g),
        overlay_channel(src.b, dst.b),
        src.a
    };
}

} // namespace tachyon::color
