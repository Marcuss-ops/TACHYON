#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"

#include <functional>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace tachyon::renderer2d {

namespace {

using SamplerFn = std::function<std::pair<float,float>(int x, int y, int s, int samples)>;

SurfaceRGBA apply_accumulation_blur(
    const SurfaceRGBA& input,
    int samples,
    SamplerFn sampler
) {
    SurfaceRGBA out(input.width(), input.height());
    out.clear(Color::transparent());

    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            LinearColor acc{0.0f, 0.0f, 0.0f};
            float acc_a = 0.0f;
            for (int s = 0; s < samples; ++s) {
                auto [sx, sy] = sampler(x, y, s, samples);
                const Color sample = sample_texture_bilinear(
                    input,
                    sx / std::max(1.0f, static_cast<float>(input.width())),
                    sy / std::max(1.0f, static_cast<float>(input.height())),
                    Color::white());
                const LinearColor linear = to_linear(sample);
                acc.r += linear.r * sample.a;
                acc.g += linear.g * sample.a;
                acc.b += linear.b * sample.a;
                acc_a += sample.a;
            }
            const float inv = 1.0f / samples;
            out.set_pixel(x, y, from_linear(
                LinearColor{acc.r*inv, acc.g*inv, acc.b*inv}, acc_a*inv));
        }
    }
    return out;
}

} // namespace

SurfaceRGBA GaussianBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    return blur_surface(input, get_scalar(params, "blur_radius", 0.0f));
}

SurfaceRGBA DirectionalBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float amount = get_scalar(params, "amount", 10.0f);
    const float angle  = get_scalar(params, "angle", 0.0f) * (M_PI / 180.0f);
    const float x_dir  = std::cos(angle);
    const float y_dir  = std::sin(angle);
    const int samples  = std::max(2, static_cast<int>(std::lround(get_scalar(params, "samples", 8.0f))));
    return apply_accumulation_blur(input, samples,
        [x_dir, y_dir, amount](int x, int y, int s, int total) {
            const float t = static_cast<float>(s) / total - 0.5f;
            return std::pair<float, float>{
                static_cast<float>(x) + x_dir * amount * t,
                static_cast<float>(y) + y_dir * amount * t
            };
        });
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

    return apply_accumulation_blur(input, samples,
        [cx, cy, amount](int x, int y, int s, int total) {
            const float t = static_cast<float>(s) / (total - 1);
            return std::pair{cx + ((float)x - cx) * (1.0f + amount * t),
                             cy + ((float)y - cy) * (1.0f + amount * t)};
        });
}

} // namespace tachyon::renderer2d