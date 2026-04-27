#include <cmath>
#include <algorithm>
#include <tuple>

namespace tachyon::renderer2d {

float unpremultiply(float channel, float alpha) {
    return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
}

std::tuple<float, float, float> rgb_to_hsl(float r, float g, float b) {
    float max = std::max({r, g, b}), min = std::min({r, g, b});
    float h, s, l = (max + min) / 2.0f;
    float d = max - min;
    if (d == 0) h = 0;
    else if (max == r) h = std::fmod((g - b) / d + (g < b ? 6 : 0), 6.0f);
    else if (max == g) h = (b - r) / d + 2.0f;
    else h = (r - g) / d + 4.0f;
    h *= 60.0f;
    s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
    return {h, s, l};
}

std::tuple<float, float, float> hsl_to_rgb(float h, float s, float l) {
    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    float r, g, b;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    return {r + m, g + m, b + m};
}

float get_luma(float r, float g, float b) {
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

} // namespace tachyon::renderer2d
