#include "tachyon/renderer2d/effects/effect_common.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

Color lerp_color(Color a, Color b, float t) {
    const float clamped_t = clamp01(t);
    return Color{
        lerp(a.r, b.r, clamped_t),
        lerp(a.g, b.g, clamped_t),
        lerp(a.b, b.b, clamped_t),
        lerp(a.a, b.a, clamped_t)
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
        detail::srgb_to_linear_component(color.r),
        detail::srgb_to_linear_component(color.g),
        detail::srgb_to_linear_component(color.b)
    };
}

Color from_linear(const LinearColor& color, float alpha) {
    return Color{
        static_cast<float>(detail::linear_to_srgb_component(color.r)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(color.g)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(color.b)) / 255.0f,
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

Color hsl_to_rgb(float h, float s, float l, float alpha) {
    h = std::fmod(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    s = clamp01(s);
    l = clamp01(l);

    if (s <= 0.0f) return Color{l, l, l, alpha};

    const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    const float p = 2.0f * l - q;
    const float r = hue_to_rgb(p, q, h + 1.0f / 3.0f);
    const float g = hue_to_rgb(p, q, h);
    const float b = hue_to_rgb(p, q, h - 1.0f / 3.0f);
    return Color{
        static_cast<float>(detail::linear_to_srgb_component(r)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(g)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(b)) / 255.0f,
        alpha
    };
}

PremultipliedPixel to_premultiplied(Color color) {
    const float alpha = color.a;
    return PremultipliedPixel{ color.r * alpha, color.g * alpha, color.b * alpha, color.a };
}

Color from_premultiplied(const PremultipliedPixel& px) {
    if (px.a <= 0.0f) return Color::transparent();
    const float inv = 1.0f / px.a;
    return Color{ std::clamp(px.r * inv, 0.0f, 1.0f), std::clamp(px.g * inv, 0.0f, 1.0f), std::clamp(px.b * inv, 0.0f, 1.0f), std::clamp(px.a, 0.0f, 1.0f) };
}

Color sample_texture_bilinear(const SurfaceRGBA& texture, float u, float v, Color tint) {
    const float src_width = static_cast<float>(texture.width());
    const float src_height = static_cast<float>(texture.height());
    if (src_width <= 0.0F || src_height <= 0.0F) return Color::transparent();

    const float fx = std::clamp(u * src_width - 0.5f, 0.0f, src_width - 1.001f);
    const float fy = std::clamp(v * src_height - 0.5f, 0.0f, src_height - 1.001f);
    const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(fx));
    const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(fy));
    const std::uint32_t x1 = x0 + 1;
    const std::uint32_t y1 = y0 + 1;
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    const Color p00 = texture.get_pixel(x0, y0);
    const Color p10 = texture.get_pixel(x1, y0);
    const Color p01 = texture.get_pixel(x0, y1);
    const Color p11 = texture.get_pixel(x1, y1);

    const auto lerp_px = [](Color a, Color b, float t) {
        return Color{ a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t };
    };

    const Color p0 = lerp_px(p00, p10, tx);
    const Color p1 = lerp_px(p01, p11, tx);
    Color res = lerp_px(p0, p1, ty);
    return Color{ res.r * tint.r, res.g * tint.g, res.b * tint.b, res.a * tint.a };
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

std::vector<PremultipliedPixel> convolve_h(const std::vector<PremultipliedPixel>& in, std::uint32_t w, std::uint32_t h, const std::vector<float>& k) {
    const int r = static_cast<int>(k.size() / 2U);
    std::vector<PremultipliedPixel> out(in.size());
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            PremultipliedPixel acc{};
            for (int d = -r; d <= r; ++d) {
                const int sx = std::clamp(static_cast<int>(x) + d, 0, static_cast<int>(w) - 1);
                const float wt = k[static_cast<std::size_t>(d + r)];
                const auto& s = in[y * w + static_cast<std::uint32_t>(sx)];
                acc.r += s.r * wt; acc.g += s.g * wt; acc.b += s.b * wt; acc.a += s.a * wt;
            }
            out[y * w + x] = acc;
        }
    }
    return out;
}

std::vector<PremultipliedPixel> convolve_v(const std::vector<PremultipliedPixel>& in, std::uint32_t w, std::uint32_t h, const std::vector<float>& k) {
    const int r = static_cast<int>(k.size() / 2U);
    std::vector<PremultipliedPixel> out(in.size());
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            PremultipliedPixel acc{};
            for (int d = -r; d <= r; ++d) {
                const int sy = std::clamp(static_cast<int>(y) + d, 0, static_cast<int>(h) - 1);
                const float wt = k[static_cast<std::size_t>(d + r)];
                const auto& s = in[static_cast<std::uint32_t>(sy) * w + x];
                acc.r += s.r * wt; acc.g += s.g * wt; acc.b += s.b * wt; acc.a += s.a * wt;
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
    const auto blurred = convolve_v(convolve_h(px, input.width(), input.height(), k), input.width(), input.height(), k);
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
    const auto blurred = convolve_v(convolve_h(px, input.width(), input.height(), k), input.width(), input.height(), k);
    SurfaceRGBA out(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y)
        for (std::uint32_t x = 0; x < input.width(); ++x)
            out.set_pixel(x, y, Color{0.0f, 0.0f, 0.0f, std::clamp(blurred[y * input.width() + x].a, 0.0f, 1.0f)});
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

std::array<float, 256> build_channel_lut(std::function<float(float)> mapper) {
    std::array<float, 256> lut{};
    for (std::size_t i = 0; i < 256; ++i) {
        const float in = static_cast<float>(i) / 255.0f;
        lut[i] = std::clamp(mapper(in), 0.0f, 1.0f);
    }
    return lut;
}

SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<float, 256>& lut) {
    return transform_surface(input, [&](Color px) {
        if (px.a == 0) return Color::transparent();
        const auto lookup = [&](float channel) -> float {
            const std::size_t index = static_cast<std::size_t>(std::lround(std::clamp(channel, 0.0f, 1.0f) * 255.0f));
            return lut[index];
        };
        return Color{lookup(px.r), lookup(px.g), lookup(px.b), px.a};
    });
}

static std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

float random01(std::uint64_t seed, std::uint64_t index, std::uint64_t salt) {
    const std::uint64_t mixed = splitmix64(seed ^ (index * 0x9e3779b97f4a7c15ULL) ^ salt);
    return static_cast<float>((mixed >> 11) * (1.0 / 9007199254740992.0));
}

void draw_disk(SurfaceRGBA& surface, int cx, int cy, int radius, Color color) {
    if (radius <= 0) {
        if (cx >= 0 && cy >= 0 && static_cast<std::uint32_t>(cx) < surface.width() && static_cast<std::uint32_t>(cy) < surface.height())
            surface.blend_pixel(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), color);
        return;
    }
    const int r2 = radius * radius;
    const int min_x = std::max(0, cx - radius), max_x = std::min(static_cast<int>(surface.width()) - 1, cx + radius);
    const int min_y = std::max(0, cy - radius), max_y = std::min(static_cast<int>(surface.height()) - 1, cy + radius);
    for (int y = min_y; y <= max_y; ++y)
        for (int x = min_x; x <= max_x; ++x)
            if (((x - cx) * (x - cx) + (y - cy) * (y - cy)) <= r2)
                surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
}

} // namespace tachyon::renderer2d
