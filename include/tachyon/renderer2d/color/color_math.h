#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <algorithm>
#include <array>
#include <functional>

#include "tachyon/renderer2d/effects/color/color_types.h"

namespace tachyon::renderer2d {

float clamp01(float value);
float lerp(float a, float b, float t);

Color lerp_color(Color a, Color b, float t);

LinearColor to_linear(Color color);
Color from_linear(const LinearColor& color, float alpha);

float luminance(const Color& color);

void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l);
Color hsl_to_rgb(float h, float s, float l, float alpha);

std::array<float, 256> build_channel_lut(std::function<float(float)> mapper);
SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<float, 256>& lut);

} // namespace tachyon::renderer2d
