#include "tachyon/renderer2d/surface/surface_blur.h"
#include "tachyon/renderer2d/surface/surface_composite.h" // for to_premultiplied etc
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

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

} // namespace tachyon::renderer2d
