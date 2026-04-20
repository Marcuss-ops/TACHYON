#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/render_context.h"

#include <array>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace tachyon::renderer2d {

// Concrete implementation of EffectHost
class EffectHostImpl : public EffectHost {
public:
    void register_effect(std::string name, std::unique_ptr<Effect> effect) override {
        if (!effect) throw std::invalid_argument("effect must not be null");
        m_effects[std::move(name)] = std::move(effect);
    }
    
    bool has_effect(const std::string& name) const override {
        return m_effects.find(name) != m_effects.end();
    }
    
    SurfaceRGBA apply(const std::string& name, const SurfaceRGBA& input, const EffectParams& params) const override {
        auto it = m_effects.find(name);
        if (it == m_effects.end()) return input;
        return it->second->apply(input, params);
    }
    
    SurfaceRGBA apply_pipeline(const SurfaceRGBA& input, const std::vector<std::pair<std::string, EffectParams>>& pipeline) const override {
        SurfaceRGBA current = input;
        for (const auto& step : pipeline) {
            auto it = m_effects.find(step.first);
            if (it != m_effects.end()) {
                current = it->second->apply(current, step.second);
            }
        }
        return current;
    }
};

std::unique_ptr<EffectHost> create_effect_host() {
    return std::make_unique<EffectHostImpl>();
}

namespace {

struct LinearColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
};

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

Color lerp_color(Color a, Color b, float t) {
    const float clamped_t = clamp01(t);
    return Color{
        static_cast<std::uint8_t>(std::lround(lerp(static_cast<float>(a.r), static_cast<float>(b.r), clamped_t))),
        static_cast<std::uint8_t>(std::lround(lerp(static_cast<float>(a.g), static_cast<float>(b.g), clamped_t))),
        static_cast<std::uint8_t>(std::lround(lerp(static_cast<float>(a.b), static_cast<float>(b.b), clamped_t))),
        static_cast<std::uint8_t>(std::lround(lerp(static_cast<float>(a.a), static_cast<float>(b.a), clamped_t)))
    };
}

bool has_scalar(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (params.scalars.find(key) != params.scalars.end()) {
            return true;
        }
    }
    return false;
}

bool has_color(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (params.colors.find(key) != params.colors.end()) {
            return true;
        }
    }
    return false;
}

float get_scalar(const EffectParams& params, const std::string& key, float fallback) {
    const auto it = params.scalars.find(key);
    return it != params.scalars.end() ? static_cast<float>(it->second) : fallback;
}

Color get_color(const EffectParams& params, const std::string& key, Color fallback) {
    const auto it = params.colors.find(key);
    if (it == params.colors.end()) {
        return fallback;
    }
    return Color{it->second.r, it->second.g, it->second.b, it->second.a};
}

LinearColor to_linear(Color color) {
    return LinearColor{
        detail::srgb_to_linear_component(static_cast<float>(color.r) / 255.0f),
        detail::srgb_to_linear_component(static_cast<float>(color.g) / 255.0f),
        detail::srgb_to_linear_component(static_cast<float>(color.b) / 255.0f)
    };
}

Color from_linear(const LinearColor& color, std::uint8_t alpha) {
    return Color{
        detail::linear_to_srgb_component(color.r),
        detail::linear_to_srgb_component(color.g),
        detail::linear_to_srgb_component(color.b),
        alpha
    };
}

float luminance(const Color& color) {
    const LinearColor linear = to_linear(color);
    return clamp01(0.2126f * linear.r + 0.7152f * linear.g + 0.0722f * linear.b);
}

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

float hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

Color hsl_to_rgb(float h, float s, float l, std::uint8_t alpha) {
    h = std::fmod(h, 1.0f);
    if (h < 0.0f) {
        h += 1.0f;
    }
    s = clamp01(s);
    l = clamp01(l);

    if (s <= 0.0f) {
        const std::uint8_t v = static_cast<std::uint8_t>(std::lround(l * 255.0f));
        return Color{v, v, v, alpha};
    }

    const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    const float p = 2.0f * l - q;
    const float r = hue_to_rgb(p, q, h + 1.0f / 3.0f);
    const float g = hue_to_rgb(p, q, h);
    const float b = hue_to_rgb(p, q, h - 1.0f / 3.0f);
    return Color{
        detail::linear_to_srgb_component(r),
        detail::linear_to_srgb_component(g),
        detail::linear_to_srgb_component(b),
        alpha
    };
}

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

struct PremultipliedPixel {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};
};

PremultipliedPixel to_premultiplied(Color color) {
    const float alpha = static_cast<float>(color.a) / 255.0f;
    return PremultipliedPixel{
        static_cast<float>(color.r) * alpha,
        static_cast<float>(color.g) * alpha,
        static_cast<float>(color.b) * alpha,
        static_cast<float>(color.a)
    };
}

Color from_premultiplied(const PremultipliedPixel& px) {
    if (px.a <= 0.0f) return Color::transparent();
    const float inv = 255.0f / px.a;
    return Color{
        static_cast<std::uint8_t>(std::clamp(std::lround(px.r * inv), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(px.g * inv), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(px.b * inv), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(px.a), 0L, 255L))
    };
}

std::vector<float> gaussian_kernel(float sigma) {
    const int radius = std::max(1, static_cast<int>(std::ceil(sigma * 3.0f)));
    std::vector<float> kernel(static_cast<std::size_t>(radius * 2 + 1), 0.0f);
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        const float w = std::exp(-(static_cast<float>(i * i)) / (2.0f * sigma * sigma));
        kernel[static_cast<std::size_t>(i + radius)] = w;
        sum += w;
    }
    for (float& w : kernel) w /= sum;
    return kernel;
}

std::vector<PremultipliedPixel> convolve_h(const std::vector<PremultipliedPixel>& in,
                                            std::uint32_t w, std::uint32_t h,
                                            const std::vector<float>& k) {
    const int r = static_cast<int>(k.size() / 2U);
    std::vector<PremultipliedPixel> out(in.size());
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            PremultipliedPixel acc{};
            for (int d = -r; d <= r; ++d) {
                const int sx = std::clamp(static_cast<int>(x) + d, 0, static_cast<int>(w) - 1);
                const float wt = k[static_cast<std::size_t>(d + r)];
                const auto& s = in[y * w + static_cast<std::uint32_t>(sx)];
                acc.r += s.r * wt; acc.g += s.g * wt;
                acc.b += s.b * wt; acc.a += s.a * wt;
            }
            out[y * w + x] = acc;
        }
    }
    return out;
}

std::vector<PremultipliedPixel> convolve_v(const std::vector<PremultipliedPixel>& in,
                                            std::uint32_t w, std::uint32_t h,
                                            const std::vector<float>& k) {
    const int r = static_cast<int>(k.size() / 2U);
    std::vector<PremultipliedPixel> out(in.size());
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            PremultipliedPixel acc{};
            for (int d = -r; d <= r; ++d) {
                const int sy = std::clamp(static_cast<int>(y) + d, 0, static_cast<int>(h) - 1);
                const float wt = k[static_cast<std::size_t>(d + r)];
                const auto& s = in[static_cast<std::uint32_t>(sy) * w + x];
                acc.r += s.r * wt; acc.g += s.g * wt;
                acc.b += s.b * wt; acc.a += s.a * wt;
            }
            out[y * w + x] = acc;
        }
    }
    return out;
}

SurfaceRGBA blur_surface(const SurfaceRGBA& input, float sigma) {
    if (sigma <= 0.0f || input.width() == 0U || input.height() == 0U) return input;
    const auto k = gaussian_kernel(sigma);
    std::vector<PremultipliedPixel> px;
    px.reserve(static_cast<std::size_t>(input.width()) * input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y)
        for (std::uint32_t x = 0; x < input.width(); ++x)
            px.push_back(to_premultiplied(input.get_pixel(x, y)));
    const auto blurred = convolve_v(convolve_h(px, input.width(), input.height(), k),
                                    input.width(), input.height(), k);
    SurfaceRGBA out(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y)
        for (std::uint32_t x = 0; x < input.width(); ++x)
            out.set_pixel(x, y, from_premultiplied(blurred[y * input.width() + x]));
    return out;
}

SurfaceRGBA blur_alpha_mask(const SurfaceRGBA& input, float sigma) {
    if (sigma <= 0.0f || input.width() == 0U || input.height() == 0U) return input;
    const auto k = gaussian_kernel(sigma);
    std::vector<PremultipliedPixel> px;
    px.reserve(static_cast<std::size_t>(input.width()) * input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y)
        for (std::uint32_t x = 0; x < input.width(); ++x)
            px.push_back({0.0f, 0.0f, 0.0f, static_cast<float>(input.get_pixel(x, y).a)});
    const auto blurred = convolve_v(convolve_h(px, input.width(), input.height(), k),
                                    input.width(), input.height(), k);
    SurfaceRGBA out(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y)
        for (std::uint32_t x = 0; x < input.width(); ++x)
            out.set_pixel(x, y, Color{0, 0, 0,
                static_cast<std::uint8_t>(std::clamp(std::lround(blurred[y * input.width() + x].a), 0L, 255L))});
    return out;
}

void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy) {
    for (std::uint32_t y = 0; y < src.height(); ++y) {
        const int ty = static_cast<int>(y) + oy;
        if (ty < 0 || static_cast<std::uint32_t>(ty) >= dst.height()) continue;
        for (std::uint32_t x = 0; x < src.width(); ++x) {
            const int tx = static_cast<int>(x) + ox;
            if (tx < 0 || static_cast<std::uint32_t>(tx) >= dst.width()) continue;
            dst.blend_pixel(static_cast<std::uint32_t>(tx), static_cast<std::uint32_t>(ty), src.get_pixel(x, y));
        }
    }
}

std::array<std::uint8_t, 256> build_channel_lut(std::function<float(float)> mapper) {
    std::array<std::uint8_t, 256> lut{};
    for (std::size_t i = 0; i < 256; ++i) {
        const float in = static_cast<float>(i) / 255.0f;
        lut[i] = static_cast<std::uint8_t>(std::lround(std::clamp(mapper(in), 0.0f, 1.0f) * 255.0f));
    }
    return lut;
}

SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<std::uint8_t, 256>& lut) {
    return transform_surface(input, [&](Color px) {
        if (px.a == 0) return Color::transparent();
        return Color{lut[px.r], lut[px.g], lut[px.b], px.a};
    });
}

} // namespace

SurfaceRGBA GaussianBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    return blur_surface(input, get_scalar(params, "blur_radius", 0.0f));
}

SurfaceRGBA DropShadowEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float blur_radius = get_scalar(params, "blur_radius", 4.0f);
    const int offset_x     = static_cast<int>(get_scalar(params, "offset_x", 4.0f));
    const int offset_y     = static_cast<int>(get_scalar(params, "offset_y", 4.0f));
    const Color shadow_color = get_color(params, "shadow_color", Color{0, 0, 0, 160});

    SurfaceRGBA shadow = blur_alpha_mask(input, blur_radius);
    for (std::uint32_t y = 0; y < shadow.height(); ++y) {
        for (std::uint32_t x = 0; x < shadow.width(); ++x) {
            const std::uint8_t alpha = shadow.get_pixel(x, y).a;
            if (alpha == 0U) continue;
            const std::uint32_t sa = static_cast<std::uint32_t>(alpha) * shadow_color.a / 255U;
            shadow.set_pixel(x, y, Color{shadow_color.r, shadow_color.g, shadow_color.b,
                                         static_cast<std::uint8_t>(sa)});
        }
    }

    SurfaceRGBA out(input.width(), input.height());
    composite_with_offset(out, shadow, offset_x, offset_y);
    composite_with_offset(out, input, 0, 0);
    return out;
}

SurfaceRGBA GlowEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float radius   = get_scalar(params, "radius", get_scalar(params, "blur_radius", 4.0f));
    const float strength = std::max(0.0f, get_scalar(params, "strength", 1.0f));
    if (input.width() == 0U || input.height() == 0U) return input;

    const SurfaceRGBA blurred = blur_surface(input, radius);
    SurfaceRGBA out(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const Color base = input.get_pixel(x, y);
            const Color glow = blurred.get_pixel(x, y);
            out.set_pixel(x, y, Color{
                static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(base.r) + static_cast<float>(glow.r) * strength), 0L, 255L)),
                static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(base.g) + static_cast<float>(glow.g) * strength), 0L, 255L)),
                static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(base.b) + static_cast<float>(glow.b) * strength), 0L, 255L)),
                base.a
            });
        }
    }
    return out;
}

SurfaceRGBA LevelsEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float in_black  = clamp01(get_scalar(params, "input_black",  0.0f) / 255.0f);
    const float in_white  = clamp01(get_scalar(params, "input_white",  1.0f));
    const float gamma     = std::max(0.0001f, get_scalar(params, "gamma", 1.0f));
    const float out_black = clamp01(get_scalar(params, "output_black", 0.0f) / 255.0f);
    const float out_white = clamp01(get_scalar(params, "output_white", 1.0f));
    const float in_range  = in_white - in_black;
    const float out_range = out_white - out_black;
    if (in_range <= 0.0f || out_range <= 0.0f) return input;

    const auto lut = build_channel_lut([&](float v) {
        const float n = clamp01((v - in_black) / in_range);
        return out_black + std::pow(n, 1.0f / gamma) * out_range;
    });
    return apply_channel_lut(input, lut);
}

SurfaceRGBA CurvesEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const bool active = has_scalar(params, {"master_gamma", "gamma", "contrast", "pivot", "red_gamma", "green_gamma", "blue_gamma"});
    if (!active) {
        return input;
    }

    const float master_gamma = std::max(0.01f, get_scalar(params, "master_gamma", 1.0f));
    const float red_gamma = std::max(0.01f, get_scalar(params, "red_gamma", 1.0f));
    const float green_gamma = std::max(0.01f, get_scalar(params, "green_gamma", 1.0f));
    const float blue_gamma = std::max(0.01f, get_scalar(params, "blue_gamma", 1.0f));
    const float contrast = std::max(0.0f, get_scalar(params, "contrast", 1.0f));
    const float pivot = clamp01(get_scalar(params, "pivot", 0.5f));
    const float mix = clamp01(get_scalar(params, "amount", 1.0f));

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) {
            return pixel;
        }

        LinearColor linear = to_linear(pixel);
        auto curve = [&](float value, float gamma) {
            const float adjusted = std::pow(clamp01(value), 1.0f / gamma);
            return clamp01((adjusted - pivot) * contrast + pivot);
        };

        LinearColor curved{
            curve(linear.r, master_gamma * red_gamma),
            curve(linear.g, master_gamma * green_gamma),
            curve(linear.b, master_gamma * blue_gamma)
        };

        Color result = from_linear(curved, pixel.a);
        return mix < 1.0f ? lerp_color(pixel, result, mix) : result;
    });
}

SurfaceRGBA FillEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    if (!has_color(params, {"fill_color", "color", "fill"})) {
        return input;
    }

    const Color fill = get_color(params, "fill_color", get_color(params, "color", get_color(params, "fill", Color::white())));
    const float opacity = clamp01(get_scalar(params, "opacity", 1.0f));
    const float mix = clamp01(get_scalar(params, "amount", 1.0f));

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) {
            return pixel;
        }

        Color result{
            fill.r,
            fill.g,
            fill.b,
            static_cast<std::uint8_t>(std::lround(static_cast<float>(pixel.a) * (static_cast<float>(fill.a) / 255.0f) * opacity))
        };
        return mix < 1.0f ? lerp_color(pixel, result, mix) : result;
    });
}

SurfaceRGBA TintEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const bool active = has_color(params, {"map_black_to", "map_white_to", "black", "white"}) || has_scalar(params, {"amount", "tint_amount"});
    if (!active) {
        return input;
    }

    const Color black = get_color(params, "map_black_to", get_color(params, "black", Color::black()));
    const Color white = get_color(params, "map_white_to", get_color(params, "white", Color::white()));
    const float mix = clamp01(get_scalar(params, "amount", get_scalar(params, "tint_amount", 1.0f)));

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) {
            return pixel;
        }

        const float mono = luminance(pixel);
        const Color tinted = lerp_color(black, white, mono);
        return mix < 1.0f ? lerp_color(pixel, tinted, mix) : tinted;
    });
}

SurfaceRGBA HueSaturationEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const bool active = has_scalar(params, {"hue_shift", "hue", "saturation", "saturation_scale", "lightness", "amount"});
    if (!active) {
        return input;
    }

    const float hue_shift = get_scalar(params, "hue_shift", get_scalar(params, "hue", 0.0f)) / 360.0f;
    const float saturation_scale = std::max(0.0f, get_scalar(params, "saturation", get_scalar(params, "saturation_scale", 1.0f)));
    const float lightness_offset = get_scalar(params, "lightness", 0.0f);
    const float mix = clamp01(get_scalar(params, "amount", 1.0f));

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) {
            return pixel;
        }

        const LinearColor linear = to_linear(pixel);
        float h = 0.0f;
        float s = 0.0f;
        float l = 0.0f;
        rgb_to_hsl(linear.r, linear.g, linear.b, h, s, l);
        const Color adjusted = hsl_to_rgb(h + hue_shift, s * saturation_scale, clamp01(l + lightness_offset), pixel.a);
        return mix < 1.0f ? lerp_color(pixel, adjusted, mix) : adjusted;
    });
}

SurfaceRGBA ColorBalanceEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const bool active = has_color(params, {"shadows", "midtones", "highlights"}) ||
                        has_scalar(params, {"shadows_amount", "midtones_amount", "highlights_amount", "amount"});
    if (!active) {
        return input;
    }

    const Color shadows = get_color(params, "shadows", Color{128, 128, 128, 255});
    const Color midtones = get_color(params, "midtones", Color{128, 128, 128, 255});
    const Color highlights = get_color(params, "highlights", Color{128, 128, 128, 255});
    const float shadows_amount = get_scalar(params, "shadows_amount", 0.0f);
    const float midtones_amount = get_scalar(params, "midtones_amount", 0.0f);
    const float highlights_amount = get_scalar(params, "highlights_amount", 0.0f);
    const float mix = clamp01(get_scalar(params, "amount", 1.0f));

    auto color_offset = [](Color color) {
        return LinearColor{
            (static_cast<float>(color.r) - 128.0f) / 255.0f,
            (static_cast<float>(color.g) - 128.0f) / 255.0f,
            (static_cast<float>(color.b) - 128.0f) / 255.0f
        };
    };

    const LinearColor shadow_offset = color_offset(shadows);
    const LinearColor midtone_offset = color_offset(midtones);
    const LinearColor highlight_offset = color_offset(highlights);

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) {
            return pixel;
        }

        LinearColor linear = to_linear(pixel);
        const float lum = luminance(pixel);
        const float shadow_weight = clamp01(1.0f - lum * 2.0f);
        const float highlight_weight = clamp01(lum * 2.0f - 1.0f);
        const float midtone_weight = 1.0f - std::abs(lum - 0.5f) * 2.0f;

        linear.r = clamp01(linear.r + shadow_offset.r * shadow_weight * shadows_amount + midtone_offset.r * midtone_weight * midtones_amount + highlight_offset.r * highlight_weight * highlights_amount);
        linear.g = clamp01(linear.g + shadow_offset.g * shadow_weight * shadows_amount + midtone_offset.g * midtone_weight * midtones_amount + highlight_offset.g * highlight_weight * highlights_amount);
        linear.b = clamp01(linear.b + shadow_offset.b * shadow_weight * shadows_amount + midtone_offset.b * midtone_weight * midtones_amount + highlight_offset.b * highlight_weight * highlights_amount);

        const Color adjusted = from_linear(linear, pixel.a);
        return mix < 1.0f ? lerp_color(pixel, adjusted, mix) : adjusted;
    });
}

SurfaceRGBA LUTEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const bool active = has_scalar(params, {"lut_amount", "exposure", "contrast", "gamma", "saturation", "temperature", "tint"});
    if (!active) {
        return input;
    }

    const float amount = clamp01(get_scalar(params, "lut_amount", 1.0f));
    const float exposure = get_scalar(params, "exposure", 0.0f);
    const float contrast = std::max(0.0f, get_scalar(params, "contrast", 1.0f));
    const float gamma = std::max(0.01f, get_scalar(params, "gamma", 1.0f));
    const float saturation = std::max(0.0f, get_scalar(params, "saturation", 1.0f));
    const float temperature = get_scalar(params, "temperature", 0.0f);
    const float tint = get_scalar(params, "tint", 0.0f);

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) {
            return pixel;
        }

        LinearColor linear = to_linear(pixel);
        linear.r = std::pow(clamp01(linear.r * std::pow(2.0f, exposure)), 1.0f / gamma);
        linear.g = std::pow(clamp01(linear.g * std::pow(2.0f, exposure)), 1.0f / gamma);
        linear.b = std::pow(clamp01(linear.b * std::pow(2.0f, exposure)), 1.0f / gamma);

        const float lum = clamp01(0.2126f * linear.r + 0.7152f * linear.g + 0.0722f * linear.b);
        linear.r = clamp01((linear.r - 0.5f) * contrast + 0.5f);
        linear.g = clamp01((linear.g - 0.5f) * contrast + 0.5f);
        linear.b = clamp01((linear.b - 0.5f) * contrast + 0.5f);

        linear.r = lerp(lum, linear.r, saturation);
        linear.g = lerp(lum, linear.g, saturation);
        linear.b = lerp(lum, linear.b, saturation);

        linear.r = clamp01(linear.r + temperature * 0.05f);
        linear.b = clamp01(linear.b - temperature * 0.05f);
        linear.g = clamp01(linear.g + tint * 0.05f);

        const Color adjusted = from_linear(linear, pixel.a);
        return amount < 1.0f ? lerp_color(pixel, adjusted, amount) : adjusted;
    });
}

void EffectHost::register_builtins(EffectHost& host) {
    host.register_effect("gaussian_blur", std::make_unique<GaussianBlurEffect>());
    host.register_effect("drop_shadow", std::make_unique<DropShadowEffect>());
    host.register_effect("glow", std::make_unique<GlowEffect>());
    host.register_effect("levels", std::make_unique<LevelsEffect>());
    host.register_effect("curves", std::make_unique<CurvesEffect>());
    host.register_effect("fill", std::make_unique<FillEffect>());
    host.register_effect("tint", std::make_unique<TintEffect>());
    host.register_effect("hue_saturation", std::make_unique<HueSaturationEffect>());
    host.register_effect("color_balance", std::make_unique<ColorBalanceEffect>());
    host.register_effect("lut", std::make_unique<LUTEffect>());
}

} // namespace tachyon::renderer2d
