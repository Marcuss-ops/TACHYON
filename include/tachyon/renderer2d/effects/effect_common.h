#pragma once

#include "tachyon/renderer2d/effects/effect_params.h"
#include "tachyon/renderer2d/effects/color_types.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon::renderer2d {

float clamp01(float value);
float lerp(float a, float b, float t);
Color lerp_color(Color a, Color b, float t);

bool has_scalar(const EffectParams& params, const std::initializer_list<const char*> keys);
bool has_color(const EffectParams& params, const std::initializer_list<const char*> keys);
float get_scalar(const EffectParams& params, const std::string& key, float fallback);
Color get_color(const EffectParams& params, const std::string& key, Color fallback);

LinearColor to_linear(Color color);
Color from_linear(const LinearColor& color, float alpha);
float luminance(const Color& color);

void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l);
float hue_to_rgb(float p, float q, float t);
Color hsl_to_rgb(float h, float s, float l, float alpha);

template <typename Fn>
SurfaceRGBA transform_surface(const SurfaceRGBA& input, Fn&& fn) {
    SurfaceRGBA output(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            output.set_pixel(x, y, fn(input.get_pixel(x, y)));
        }
    }
    return output;
}

PremultipliedPixel to_premultiplied(Color color);
Color from_premultiplied(const PremultipliedPixel& px);

Color sample_texture_bilinear(const SurfaceRGBA& texture, float u, float v, Color tint);

std::vector<float> gaussian_kernel(float sigma);
std::vector<PremultipliedPixel> convolve_h(const std::vector<PremultipliedPixel>& in,
                                            std::uint32_t w, std::uint32_t h,
                                            const std::vector<float>& k);
std::vector<PremultipliedPixel> convolve_v(const std::vector<PremultipliedPixel>& in,
                                            std::uint32_t w, std::uint32_t h,
                                            const std::vector<float>& k);

SurfaceRGBA blur_surface(const SurfaceRGBA& input, float sigma);
SurfaceRGBA blur_alpha_mask(const SurfaceRGBA& input, float sigma);

void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy);

std::array<float, 256> build_channel_lut(std::function<float(float)> mapper);
SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<float, 256>& lut);

} // namespace tachyon::renderer2d
