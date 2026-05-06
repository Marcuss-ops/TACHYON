#include "tachyon/color/color_math.h"
#include <algorithm>
#include <cmath>

namespace tachyon::color {

void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l) {
    const float max_v = std::max({r, g, b});
    const float min_v = std::min({r, g, b});
    l = (max_v + min_v) * 0.5f;

    if (max_v == min_v) {
        h = 0.0f;
        s = 0.0f;
        return;
    }

    const float d = max_v - min_v;
    s = l > 0.5f ? d / (2.0f - max_v - min_v) : d / (max_v + min_v);

    if (max_v == r) {
        h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    } else if (max_v == g) {
        h = (b - r) / d + 2.0f;
    } else {
        h = (r - g) / d + 4.0f;
    }

    h /= 6.0f;
}

static float hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

LinearRGBA hsl_to_rgb(float h, float s, float l, float a) {
    h = std::fmod(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    
    s = std::clamp(s, 0.0f, 1.0f);
    l = std::clamp(l, 0.0f, 1.0f);

    if (s <= 0.0f) return LinearRGBA{l, l, l, a};

    const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    const float p = 2.0f * l - q;
    
    return LinearRGBA{
        hue_to_rgb(p, q, h + 1.0f / 3.0f),
        hue_to_rgb(p, q, h),
        hue_to_rgb(p, q, h - 1.0f / 3.0f),
        a
    };
}

} // namespace tachyon::color
