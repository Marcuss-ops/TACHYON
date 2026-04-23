#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"

namespace tachyon::renderer2d {

SurfaceRGBA GaussianBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    return blur_surface(input, get_scalar(params, "blur_radius", 0.0f));
}

SurfaceRGBA DirectionalBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float amount = std::max(0.0f, get_scalar(params, "amount", get_scalar(params, "blur_radius", 0.0f)));
    const float x_dir = get_scalar(params, "x_direction", 1.0f);
    const float y_dir = get_scalar(params, "y_direction", 0.0f);
    const int samples = std::max(2, static_cast<int>(std::lround(get_scalar(params, "samples", 8.0f))));
    if (amount <= 0.0f || input.width() == 0U || input.height() == 0U) {
        return input;
    }
    
    const float inv_samples = 1.0f / static_cast<float>(samples);
    SurfaceRGBA out(input.width(), input.height());
    out.clear(Color::transparent());
    
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            LinearColor acc{0.0f, 0.0f, 0.0f};
            float acc_a = 0.0f;
            for (int s = 0; s < samples; ++s) {
                const float t = static_cast<float>(s) * inv_samples - 0.5f;
                const float sample_x = static_cast<float>(x) + x_dir * amount * t;
                const float sample_y = static_cast<float>(y) + y_dir * amount * t;
                const Color sample = sample_texture_bilinear(
                    input,
                    sample_x / std::max(1.0f, static_cast<float>(input.width())),
                    sample_y / std::max(1.0f, static_cast<float>(input.height())),
                    Color::white());
                const LinearColor linear = to_linear(sample);
                acc.r += linear.r;
                acc.g += linear.g;
                acc.b += linear.b;
                acc_a += sample.a;
            }
            const float inv = 1.0f / static_cast<float>(samples);
            out.set_pixel(x, y, from_linear(LinearColor{acc.r * inv, acc.g * inv, acc.b * inv}, acc_a * inv));
        }
    }
    return out;
}

SurfaceRGBA RadialBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float amount = std::max(0.0f, get_scalar(params, "amount", get_scalar(params, "blur_radius", 0.0f)));
    const float center_x = get_scalar(params, "center_x", 0.5f);
    const float center_y = get_scalar(params, "center_y", 0.5f);
    const int samples = std::max(2, static_cast<int>(std::lround(get_scalar(params, "samples", 8.0f))));
    if (amount <= 0.0f || input.width() == 0U || input.height() == 0U) {
        return input;
    }
    
    const float cx = center_x * static_cast<float>(input.width());
    const float cy = center_y * static_cast<float>(input.height());
    SurfaceRGBA out(input.width(), input.height());
    out.clear(Color::transparent());
    
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            LinearColor acc{0.0f, 0.0f, 0.0f};
            float acc_a = 0.0f;
            for (int s = 0; s < samples; ++s) {
                const float t = static_cast<float>(s) / static_cast<float>(samples - 1);
                const float sample_x = cx + (static_cast<float>(x) - cx) * (1.0f + amount * t);
                const float sample_y = cy + (static_cast<float>(y) - cy) * (1.0f + amount * t);
                const Color sample = sample_texture_bilinear(
                    input,
                    sample_x / std::max(1.0f, static_cast<float>(input.width())),
                    sample_y / std::max(1.0f, static_cast<float>(input.height())),
                    Color::white());
                const LinearColor linear = to_linear(sample);
                acc.r += linear.r;
                acc.g += linear.g;
                acc.b += linear.b;
                acc_a += sample.a;
            }
            const float inv = 1.0f / static_cast<float>(samples);
            out.set_pixel(x, y, from_linear(LinearColor{acc.r * inv, acc.g * inv, acc.b * inv}, acc_a * inv));
        }
    }
    return out;
}

} // namespace tachyon::renderer2d