#include "tachyon/renderer2d/color/color_math.h"
#include <algorithm>
#include <tuple>

namespace tachyon::renderer2d {

float unpremultiply(float channel, float alpha) {
    return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
}

float get_luma(float r, float g, float b) {
    return luma_rec601(Color{r, g, b, 1.0f});
}

} // namespace tachyon::renderer2d
