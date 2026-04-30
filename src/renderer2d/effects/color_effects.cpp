#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"
#include "tachyon/renderer2d/color/lut3d.h"

namespace tachyon::renderer2d {

SurfaceRGBA LevelsEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA CurvesEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA FillEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const Color fill_color = get_color(params, "color", Color{1.0f, 1.0f, 1.0f, 1.0f});
    SurfaceRGBA out = input;
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            const Color px = out.get_pixel(x, y);
            if (px.a <= 0.0f) continue;
            // Preserve alpha, replace RGB with fill color
            out.set_pixel(x, y, Color{fill_color.r, fill_color.g, fill_color.b, px.a});
        }
    }
    return out;
}

SurfaceRGBA TintEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA HueSaturationEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA ColorBalanceEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA LUTEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA Lut3DCubeEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA ChromaKeyEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA LightWrapEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA MatteRefinementEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA VectorBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    (void)params;
    return input; // Stub implementation
}

SurfaceRGBA MotionBlur2DEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const int samples = static_cast<int>(get_scalar(params, "samples", 8.0f));
    const float angle_deg = get_scalar(params, "angle", 0.0f);
    const float distance = get_scalar(params, "distance", 10.0f);
    const float shutter_angle = get_scalar(params, "shutter_angle", 180.0f) / 360.0f;

    if (samples <= 1 || distance <= 0.0f) return input;

    const float angle_rad = angle_deg * 3.14159265358979323846f / 180.0f;
    const float dx = std::cos(angle_rad) * distance * shutter_angle;
    const float dy = std::sin(angle_rad) * distance * shutter_angle;

    SurfaceRGBA out = input;
    out.clear(Color::transparent());

    for (int s = 0; s < samples; ++s) {
        const float t = (s / static_cast<float>(samples - 1)) - 0.5f;
        const int offset_x = static_cast<int>(dx * t);
        const int offset_y = static_cast<int>(dy * t);
        const float weight = 1.0f / samples;

        for (std::uint32_t y = 0; y < input.height(); ++y) {
            for (std::uint32_t x = 0; x < input.width(); ++x) {
                int src_x = static_cast<int>(x) - offset_x;
                int src_y = static_cast<int>(y) - offset_y;
                if (src_x >= 0 && src_x < static_cast<int>(input.width()) &&
                    src_y >= 0 && src_y < static_cast<int>(input.height())) {
                    Color src = input.get_pixel(static_cast<std::uint32_t>(src_x),
                                                static_cast<std::uint32_t>(src_y));
                    src.a *= weight;
                    Color dst = out.get_pixel(x, y);
                    out.set_pixel(x, y, Color{ dst.r + src.r, dst.g + src.g, dst.b + src.b, dst.a + src.a });
                }
            }
        }
    }

    // Normalize alpha
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            if (px.a > 0.0f) {
                float inv_a = 1.0f / px.a;
                out.set_pixel(x, y, Color{ px.r * inv_a, px.g * inv_a, px.b * inv_a, 1.0f });
            }
        }
    }

    return out;
}

} // namespace tachyon::renderer2d