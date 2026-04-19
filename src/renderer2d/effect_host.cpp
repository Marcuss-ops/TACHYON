#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/render_context.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tachyon::renderer2d {
namespace {

Color premultiply_color(Color color) {
    color.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.r) * color.a + 127U) / 255U);
    color.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.g) * color.a + 127U) / 255U);
    color.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.b) * color.a + 127U) / 255U);
    return color;
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

Color from_premultiplied(const PremultipliedPixel& pixel) {
    if (pixel.a <= 0.0f) {
        return Color::transparent();
    }

    const float inv_alpha = 255.0f / pixel.a;
    return Color{
        static_cast<std::uint8_t>(std::clamp(std::lround(pixel.r * inv_alpha), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(pixel.g * inv_alpha), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(pixel.b * inv_alpha), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(pixel.a), 0L, 255L))
    };
}

template <typename Fn>
std::array<std::uint8_t, 256> build_channel_lut(Fn&& mapper) {
    std::array<std::uint8_t, 256> lut{};
    for (std::size_t index = 0; index < lut.size(); ++index) {
        const float input = static_cast<float>(index) / 255.0f;
        const float output = std::clamp(mapper(input), 0.0f, 1.0f);
        lut[index] = static_cast<std::uint8_t>(std::lround(output * 255.0f));
    }
    return lut;
}

SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<std::uint8_t, 256>& lut) {
    SurfaceRGBA output(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const Color base = input.get_pixel(x, y);
            if (base.a == 0U) {
                output.set_pixel(x, y, Color::transparent());
                continue;
            }

            output.set_pixel(x, y, Color{
                lut[base.r],
                lut[base.g],
                lut[base.b],
                base.a
            });
        }
    }
    return output;
}

std::vector<float> gaussian_kernel(float sigma) {
    const int radius = std::max(1, static_cast<int>(std::ceil(sigma * 3.0f)));
    std::vector<float> kernel(static_cast<std::size_t>(radius * 2 + 1), 0.0f);
    const float two_sigma_sq = 2.0f * sigma * sigma;
    float sum = 0.0f;

    for (int index = -radius; index <= radius; ++index) {
        const float weight = std::exp(-(static_cast<float>(index * index)) / two_sigma_sq);
        kernel[static_cast<std::size_t>(index + radius)] = weight;
        sum += weight;
    }

    for (float& weight : kernel) {
        weight /= sum;
    }

    return kernel;
}

std::vector<PremultipliedPixel> convolve_horizontal(
    const std::vector<PremultipliedPixel>& input,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<float>& kernel) {

    const int radius = static_cast<int>(kernel.size() / 2U);
    std::vector<PremultipliedPixel> output(input.size());

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            PremultipliedPixel accum{};
            for (int offset = -radius; offset <= radius; ++offset) {
                const int sample_x = std::clamp(static_cast<int>(x) + offset, 0, static_cast<int>(width) - 1);
                const float weight = kernel[static_cast<std::size_t>(offset + radius)];
                const PremultipliedPixel& sample = input[static_cast<std::size_t>(y) * width + static_cast<std::uint32_t>(sample_x)];
                accum.r += sample.r * weight;
                accum.g += sample.g * weight;
                accum.b += sample.b * weight;
                accum.a += sample.a * weight;
            }
            output[static_cast<std::size_t>(y) * width + x] = accum;
        }
    }

    return output;
}

std::vector<PremultipliedPixel> convolve_vertical(
    const std::vector<PremultipliedPixel>& input,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<float>& kernel) {

    const int radius = static_cast<int>(kernel.size() / 2U);
    std::vector<PremultipliedPixel> output(input.size());

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            PremultipliedPixel accum{};
            for (int offset = -radius; offset <= radius; ++offset) {
                const int sample_y = std::clamp(static_cast<int>(y) + offset, 0, static_cast<int>(height) - 1);
                const float weight = kernel[static_cast<std::size_t>(offset + radius)];
                const PremultipliedPixel& sample = input[static_cast<std::size_t>(sample_y) * width + x];
                accum.r += sample.r * weight;
                accum.g += sample.g * weight;
                accum.b += sample.b * weight;
                accum.a += sample.a * weight;
            }
            output[static_cast<std::size_t>(y) * width + x] = accum;
        }
    }

    return output;
}

std::vector<PremultipliedPixel> capture_surface(const SurfaceRGBA& surface) {
    std::vector<PremultipliedPixel> pixels;
    pixels.reserve(static_cast<std::size_t>(surface.width()) * surface.height());
    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 0; x < surface.width(); ++x) {
            pixels.push_back(to_premultiplied(surface.get_pixel(x, y)));
        }
    }
    return pixels;
}

SurfaceRGBA build_surface_from_pixels(std::uint32_t width, std::uint32_t height, const std::vector<PremultipliedPixel>& pixels) {
    SurfaceRGBA output(width, height);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            output.set_pixel(x, y, from_premultiplied(pixels[static_cast<std::size_t>(y) * width + x]));
        }
    }
    return output;
}

SurfaceRGBA blur_surface(const SurfaceRGBA& input, float sigma) {
    if (sigma <= 0.0f || input.width() == 0U || input.height() == 0U) {
        return input;
    }

    const std::vector<float> kernel = gaussian_kernel(sigma);
    const std::vector<PremultipliedPixel> captured = capture_surface(input);
    const std::vector<PremultipliedPixel> horizontal = convolve_horizontal(captured, input.width(), input.height(), kernel);
    const std::vector<PremultipliedPixel> vertical = convolve_vertical(horizontal, input.width(), input.height(), kernel);
    return build_surface_from_pixels(input.width(), input.height(), vertical);
}

SurfaceRGBA blur_alpha_mask(const SurfaceRGBA& input, float sigma) {
    if (sigma <= 0.0f || input.width() == 0U || input.height() == 0U) {
        return input;
    }

    const std::vector<float> kernel = gaussian_kernel(sigma);
    std::vector<PremultipliedPixel> captured;
    captured.reserve(static_cast<std::size_t>(input.width()) * input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const Color color = input.get_pixel(x, y);
            captured.push_back(PremultipliedPixel{0.0f, 0.0f, 0.0f, static_cast<float>(color.a)});
        }
    }

    const std::vector<PremultipliedPixel> horizontal = convolve_horizontal(captured, input.width(), input.height(), kernel);
    const std::vector<PremultipliedPixel> vertical = convolve_vertical(horizontal, input.width(), input.height(), kernel);

    SurfaceRGBA output(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const PremultipliedPixel& pixel = vertical[static_cast<std::size_t>(y) * input.width() + x];
            output.set_pixel(x, y, Color{0, 0, 0, static_cast<std::uint8_t>(std::clamp(std::lround(pixel.a), 0L, 255L))});
        }
    }
    return output;
}

void composite_with_offset(SurfaceRGBA& destination, const SurfaceRGBA& source, int offset_x, int offset_y) {
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        for (std::uint32_t x = 0; x < source.width(); ++x) {
            const int target_x = static_cast<int>(x) + offset_x;
            const int target_y = static_cast<int>(y) + offset_y;
            if (target_x < 0 || target_y < 0) {
                continue;
            }
            destination.blend_pixel(static_cast<std::uint32_t>(target_x),
                                    static_cast<std::uint32_t>(target_y),
                                    source.get_pixel(x, y));
        }
    }
}

float get_scalar(const EffectParams& params, const std::string& key, float default_value) {
    const auto it = params.scalars.find(key);
    return it != params.scalars.end() ? it->second : default_value;
}

float normalize_unit_or_byte(float value, float default_unit_scale = 255.0f) {
    if (value <= 1.0f) {
        return std::clamp(value, 0.0f, 1.0f);
    }
    return std::clamp(value / default_unit_scale, 0.0f, 1.0f);
}

Color get_color(const EffectParams& params, const std::string& key, Color default_value) {
    const auto it = params.colors.find(key);
    return it != params.colors.end() ? it->second : default_value;
}

} // namespace

SurfaceRGBA GaussianBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    return blur_surface(input, get_scalar(params, "blur_radius", 0.0f));
}

SurfaceRGBA DropShadowEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float blur_radius = get_scalar(params, "blur_radius", 4.0f);
    const int offset_x = static_cast<int>(get_scalar(params, "offset_x", 4.0f));
    const int offset_y = static_cast<int>(get_scalar(params, "offset_y", 4.0f));
    Color shadow_color = get_color(params, "shadow_color", Color{0, 0, 0, 160});

    SurfaceRGBA shadow = blur_alpha_mask(input, blur_radius);
    for (std::uint32_t y = 0; y < shadow.height(); ++y) {
        for (std::uint32_t x = 0; x < shadow.width(); ++x) {
            const std::uint8_t alpha = shadow.get_pixel(x, y).a;
            if (alpha == 0U) {
                continue;
            }

            const std::uint32_t shadow_alpha = static_cast<std::uint32_t>(alpha) * shadow_color.a / 255U;
            shadow.set_pixel(x, y, Color{
                shadow_color.r,
                shadow_color.g,
                shadow_color.b,
                static_cast<std::uint8_t>(shadow_alpha)
            });
        }
    }

    SurfaceRGBA output(input.width(), input.height());
    composite_with_offset(output, shadow, offset_x, offset_y);
    composite_with_offset(output, input, 0, 0);
    return output;
}

SurfaceRGBA GlowEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float blur_radius = get_scalar(params, "radius", get_scalar(params, "blur_radius", 4.0f));
    const float strength = std::max(0.0f, get_scalar(params, "strength", 1.0f));

    if (input.width() == 0U || input.height() == 0U) {
        return input;
    }

    const SurfaceRGBA blurred = blur_surface(input, blur_radius);
    SurfaceRGBA output(input.width(), input.height());

    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const Color base = input.get_pixel(x, y);
            const Color glow = blurred.get_pixel(x, y);

            const auto add_channel = [strength](std::uint8_t lhs, std::uint8_t rhs) -> std::uint8_t {
                const float sum = static_cast<float>(lhs) + static_cast<float>(rhs) * strength;
                return static_cast<std::uint8_t>(std::clamp(std::lround(sum), 0L, 255L));
            };

            output.set_pixel(x, y, Color{
                add_channel(base.r, glow.r),
                add_channel(base.g, glow.g),
                add_channel(base.b, glow.b),
                base.a
            });
        }
    }

    return output;
}

SurfaceRGBA LevelsEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float input_black = normalize_unit_or_byte(get_scalar(params, "input_black", 0.0f));
    const float input_white = normalize_unit_or_byte(get_scalar(params, "input_white", 255.0f));
    const float gamma = std::max(0.0001f, get_scalar(params, "gamma", 1.0f));
    const float output_black = normalize_unit_or_byte(get_scalar(params, "output_black", 0.0f));
    const float output_white = normalize_unit_or_byte(get_scalar(params, "output_white", 255.0f));

    const float input_range = input_white - input_black;
    const float output_range = output_white - output_black;
    if (input_range <= 0.0f || output_range <= 0.0f) {
        return input;
    }

    const auto lut = build_channel_lut([&](float value) {
        const float normalized = std::clamp((value - input_black) / input_range, 0.0f, 1.0f);
        const float shaped = std::pow(normalized, 1.0f / gamma);
        return output_black + shaped * output_range;
    });

    return apply_channel_lut(input, lut);
}

SurfaceRGBA CurvesEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float amount = std::clamp(get_scalar(params, "amount", get_scalar(params, "strength", 1.0f)), -1.0f, 1.0f);
    if (amount == 0.0f) {
        return input;
    }

    const auto lut = build_channel_lut([&](float value) {
        const float smooth = value * value * (3.0f - 2.0f * value);
        return value + (smooth - value) * amount;
    });

    return apply_channel_lut(input, lut);
}

void EffectHost::register_effect(std::string name, std::unique_ptr<Effect> effect) {
    if (!effect) {
        throw std::invalid_argument("effect must not be null");
    }
    m_effects[std::move(name)] = std::move(effect);
}

bool EffectHost::has_effect(const std::string& name) const {
    return m_effects.find(name) != m_effects.end();
}

SurfaceRGBA EffectHost::apply(const std::string& name, const SurfaceRGBA& input, const EffectParams& params) const {
    const auto it = m_effects.find(name);
    if (it == m_effects.end()) {
        return input;
    }
    return it->second->apply(input, params);
}

SurfaceRGBA FillEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const Color fill_color = get_color(params, "color", Color::white());
    
    SurfaceRGBA output(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const Color base = input.get_pixel(x, y);
            if (base.a == 0) {
                output.set_pixel(x, y, Color::transparent());
                continue;
            }
            
            // Keep original alpha, but set RGB to fill color
            Color out = fill_color;
            out.a = base.a;
            output.set_pixel(x, y, premultiply_color(out));
        }
    }
    return output;
}

SurfaceRGBA TintEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const Color tint_color = get_color(params, "color", Color::white());
    const float amount = get_scalar(params, "amount", 1.0f);
    
    const float tr = static_cast<float>(tint_color.r) / 255.0f;
    const float tg = static_cast<float>(tint_color.g) / 255.0f;
    const float tb = static_cast<float>(tint_color.b) / 255.0f;

    SurfaceRGBA output(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            Color base = input.get_pixel(x, y);
            if (base.a == 0) {
                output.set_pixel(x, y, Color::transparent());
                continue;
            }

            // Unmultiply
            float alpha = static_cast<float>(base.a) / 255.0f;
            float r = static_cast<float>(base.r) / (255.0f * alpha);
            float g = static_cast<float>(base.g) / (255.0f * alpha);
            float b = static_cast<float>(base.b) / (255.0f * alpha);

            // Interpolate towards tint
            r = r + (r * tr - r) * amount;
            g = g + (g * tg - g) * amount;
            b = b + (b * tb - b) * amount;

            // Remultiply
            base.r = static_cast<uint8_t>(std::clamp(r * 255.0f * alpha, 0.0f, 255.0f));
            base.g = static_cast<uint8_t>(std::clamp(g * 255.0f * alpha, 0.0f, 255.0f));
            base.b = static_cast<uint8_t>(std::clamp(b * 255.0f * alpha, 0.0f, 255.0f));
            
            output.set_pixel(x, y, base);
        }
    }
    return output;
}

} // namespace tachyon::renderer2d
