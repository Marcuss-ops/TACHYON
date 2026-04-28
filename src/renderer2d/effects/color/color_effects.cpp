#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"
#include "tachyon/renderer2d/color/lut3d.h"

namespace tachyon::renderer2d {

namespace {

float apply_levels_channel(float value, float in_black, float in_white, float gamma, float out_black, float out_white) {
    float normalized = (value - in_black) / (in_white - in_black);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    float gamma_corrected = std::pow(normalized, gamma);
    return out_black + gamma_corrected * (out_white - out_black);
}

} // namespace

SurfaceRGBA LevelsEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float in_black_r = get_scalar(params, "in_black_r", 0.0f);
    const float in_white_r = get_scalar(params, "in_white_r", 1.0f);
    const float gamma_r = get_scalar(params, "gamma_r", 1.0f);
    const float out_black_r = get_scalar(params, "out_black_r", 0.0f);
    const float out_white_r = get_scalar(params, "out_white_r", 1.0f);

    const float in_black_g = get_scalar(params, "in_black_g", 0.0f);
    const float in_white_g = get_scalar(params, "in_white_g", 1.0f);
    const float gamma_g = get_scalar(params, "gamma_g", 1.0f);
    const float out_black_g = get_scalar(params, "out_black_g", 0.0f);
    const float out_white_g = get_scalar(params, "out_white_g", 1.0f);

    const float in_black_b = get_scalar(params, "in_black_b", 0.0f);
    const float in_white_b = get_scalar(params, "in_white_b", 1.0f);
    const float gamma_b = get_scalar(params, "gamma_b", 1.0f);
    const float out_black_b = get_scalar(params, "out_black_b", 0.0f);
    const float out_white_b = get_scalar(params, "out_white_b", 1.0f);

    SurfaceRGBA out = input;
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            px.r = apply_levels_channel(px.r, in_black_r, in_white_r, gamma_r, out_black_r, out_white_r);
            px.g = apply_levels_channel(px.g, in_black_g, in_white_g, gamma_g, out_black_g, out_white_g);
            px.b = apply_levels_channel(px.b, in_black_b, in_white_b, gamma_b, out_black_b, out_white_b);
            out.set_pixel(x, y, px);
        }
    }
    return out;
}

SurfaceRGBA CurvesEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float curve_r = get_scalar(params, "curve_r", 1.0f);
    const float curve_g = get_scalar(params, "curve_g", 1.0f);
    const float curve_b = get_scalar(params, "curve_b", 1.0f);
    const float curve_master = get_scalar(params, "curve_master", 1.0f);

    SurfaceRGBA out = input;
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            px.r = std::pow(std::clamp(px.r, 0.0f, 1.0f), curve_master * curve_r);
            px.g = std::pow(std::clamp(px.g, 0.0f, 1.0f), curve_master * curve_g);
            px.b = std::pow(std::clamp(px.b, 0.0f, 1.0f), curve_master * curve_b);
            out.set_pixel(x, y, px);
        }
    }
    return out;
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
    const Color tint_color = get_color(params, "tint_color", Color{0.886f, 0.745f, 0.561f, 1.0f});
    const Color black_color = get_color(params, "black_color", Color{0.2f, 0.2f, 0.2f, 1.0f});
    const float amount = get_scalar(params, "amount", 1.0f);

    SurfaceRGBA out = input;
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            float luminance = 0.299f * px.r + 0.587f * px.g + 0.114f * px.b;
            Color tinted = Color::lerp(black_color, tint_color, luminance);
            px.r = px.r + (tinted.r - px.r) * amount;
            px.g = px.g + (tinted.g - px.g) * amount;
            px.b = px.b + (tinted.b - px.b) * amount;
            out.set_pixel(x, y, px);
        }
    }
    return out;
}

SurfaceRGBA HueSaturationEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float hue_shift = get_scalar(params, "hue", 0.0f);
    const float saturation = get_scalar(params, "saturation", 1.0f);
    const float lightness = get_scalar(params, "lightness", 0.0f);

    SurfaceRGBA out = input;
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            float h, s, l;
            rgb_to_hsl(px.r, px.g, px.b, h, s, l);

            h = std::fmod(h + hue_shift / 360.0f, 1.0f);
            if (h < 0) h += 1.0f;
            s = std::clamp(s * saturation, 0.0f, 1.0f);
            l = std::clamp(l + lightness, 0.0f, 1.0f);

            Color result = hsl_to_rgb(h, s, l, px.a);
            out.set_pixel(x, y, Color{result.r, result.g, result.b, px.a});
        }
    }
    return out;
}

SurfaceRGBA ColorBalanceEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float shadows_r = get_scalar(params, "shadows_r", 0.0f);
    const float shadows_g = get_scalar(params, "shadows_g", 0.0f);
    const float shadows_b = get_scalar(params, "shadows_b", 0.0f);

    const float midtones_r = get_scalar(params, "midtones_r", 0.0f);
    const float midtones_g = get_scalar(params, "midtones_g", 0.0f);
    const float midtones_b = get_scalar(params, "midtones_b", 0.0f);

    const float highlights_r = get_scalar(params, "highlights_r", 0.0f);
    const float highlights_g = get_scalar(params, "highlights_g", 0.0f);
    const float highlights_b = get_scalar(params, "highlights_b", 0.0f);

    SurfaceRGBA out = input;
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            float luminance = 0.299f * px.r + 0.587f * px.g + 0.114f * px.b;

            float shadow_weight = std::clamp(1.0f - luminance * 2.0f, 0.0f, 1.0f);
            float highlight_weight = std::clamp(luminance * 2.0f - 1.0f, 0.0f, 1.0f);
            float midtone_weight = 1.0f - shadow_weight - highlight_weight;

            px.r += shadows_r * shadow_weight + midtones_r * midtone_weight + highlights_r * highlight_weight;
            px.g += shadows_g * shadow_weight + midtones_g * midtone_weight + highlights_g * highlight_weight;
            px.b += shadows_b * shadow_weight + midtones_b * midtone_weight + highlights_b * highlight_weight;
            px.r = std::clamp(px.r, 0.0f, 1.0f);
            px.g = std::clamp(px.g, 0.0f, 1.0f);
            px.b = std::clamp(px.b, 0.0f, 1.0f);
            out.set_pixel(x, y, px);
        }
    }
    return out;
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

