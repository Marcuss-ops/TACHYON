#include "tachyon/renderer2d/effect_host.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tachyon::renderer2d {
namespace {

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

} // namespace tachyon::renderer2d
