#include "tachyon/color/color_space.h"
#include <cmath>

namespace tachyon::color {

static float srgb_to_linear_component(float c) {
    if (c <= 0.04045f) return c / 12.92f;
    return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

static float linear_to_srgb_component(float c) {
    if (c <= 0.0031308f) return 12.92f * c;
    return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

LinearRGBA srgb_to_linear(SRGBA c) {
    return LinearRGBA{
        srgb_to_linear_component(c.r),
        srgb_to_linear_component(c.g),
        srgb_to_linear_component(c.b),
        c.a
    };
}

SRGBA linear_to_srgb(LinearRGBA c) {
    return SRGBA{
        linear_to_srgb_component(c.r),
        linear_to_srgb_component(c.g),
        linear_to_srgb_component(c.b),
        c.a
    };
}

} // namespace tachyon::color
