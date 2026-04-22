#include "tachyon/renderer2d/effects/effect_common.h"

namespace tachyon::renderer2d {

SurfaceRGBA LevelsEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float in_black  = clamp01(get_scalar(params, "input_black",  0.0f));
    const float in_white  = clamp01(get_scalar(params, "input_white",  1.0f));
    const float gamma     = std::max(0.0001f, get_scalar(params, "gamma", 1.0f));
    const float out_black = clamp01(get_scalar(params, "output_black", 0.0f));
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
            pixel.a * fill.a * opacity
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

    const Color shadows = get_color(params, "shadows", Color{0.5f, 0.5f, 0.5f, 1.0f});
    const Color midtones = get_color(params, "midtones", Color{0.5f, 0.5f, 0.5f, 1.0f});
    const Color highlights = get_color(params, "highlights", Color{0.5f, 0.5f, 0.5f, 1.0f});
    const float shadows_amount = get_scalar(params, "shadows_amount", 0.0f);
    const float midtones_amount = get_scalar(params, "midtones_amount", 0.0f);
    const float highlights_amount = get_scalar(params, "highlights_amount", 0.0f);
    const float mix = clamp01(get_scalar(params, "amount", 1.0f));

    auto color_offset = [](Color color) {
        return LinearColor{
            color.r - 0.5f,
            color.g - 0.5f,
            color.b - 0.5f
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

SurfaceRGBA Lut3DCubeEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    // Retrieve the .cube file path from the strings map
    const auto path_it = params.strings.find("lut_path");
    if (path_it == params.strings.end() || path_it->second.empty()) {
        return input;
    }

    const float amount = clamp01(get_scalar(params, "lut_amount", 1.0f));
    if (amount <= 0.0f) {
        return input;
    }

    // Load (and cache) the LUT — per-call cache keyed by path
    Lut3D lut;
    {
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        const auto cache_it = m_lut_cache.find(path_it->second);
        if (cache_it != m_lut_cache.end()) {
            lut = cache_it->second;
        } else {
            auto load_result = load_lut_cube(std::filesystem::path(path_it->second));
            if (!load_result.ok() || !load_result.value.has_value()) {
                // Failed to load — return input unchanged rather than crashing
                return input;
            }
            lut = std::move(*load_result.value);
            m_lut_cache[path_it->second] = lut;
        }
    }

    if (!lut.is_valid()) {
        return input;
    }

    return transform_surface(input, [&](Color pixel) {
        if (pixel.a == 0) return pixel;
        const Color graded = apply_lut3d(lut, pixel);
        return amount < 1.0f ? lerp_color(pixel, graded, amount) : graded;
    });
}

} // namespace tachyon::renderer2d
