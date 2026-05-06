#include "tachyon/renderer2d/color/color_math.h"
#include <algorithm>
#include <tuple>

namespace tachyon::renderer2d {

float unpremultiply(float channel, float alpha) {
    return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
}

std::tuple<float, float, float> rgb_to_hsl(float r, float g, float b) {
    float h, s, l;
    renderer2d::rgb_to_hsl(r, g, b, h, s, l);
    // Note: older callers might expect 0-360 for H, but unified HSL uses 0-1.
    // If we want to keep 0-360 here for compatibility:
    return {h * 360.0f, s, l};
}

std::tuple<float, float, float> hsl_to_rgb(float h, float s, float l) {
    // Input h is 0-360 here
    Color c = renderer2d::hsl_to_rgb(h / 360.0f, s, l);
    return {c.r, c.g, c.b};
}

float get_luma(float r, float g, float b) {
    return luma_rec601(Color{r, g, b, 1.0f});
}

} // namespace tachyon::renderer2d
