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

// Professional Chroma Key – CbCr screen-matte approach
//
// Stage 1 – Screen matte: compute a normalised "similarity" value in the
//            Cb/Cr chrominance plane so that luma does NOT drive keying.
// Stage 2 – Clip: crush the matte black and white levels.
// Stage 3 – Matte erosion: shrink/contract the alpha map.
// Stage 4 – Matte feather: blur the alpha map for smooth edges.
// Stage 5 – Despill: suppress residual key-colour bleed in the foreground.
//
// Params:
//   key_color    (color)  – sampled screen colour, linear floating-point.
//   similarity   (scalar) – normalised match radius in CbCr space  [0..1], default 0.40
//   softness     (scalar) – transition zone around the matte edge  [0..1], default 0.10
//   clip_black   (scalar) – crush values below this to solid 0     [0..1], default 0.00
//   clip_white   (scalar) – crush values above this to solid 1     [0..1], default 1.00
//   matte_erosion(scalar) – shrink matte by N pixels               [0..∞), default 0.0
//   matte_feather(scalar) – gaussian blur radius for matte edges   [0..∞), default 0.0
//   spill        (scalar) – despill strength                       [0..1], default 0.5
//   despill_bias (scalar) – warm/cool despill balance              [0..1], default 0.5

SurfaceRGBA ChromaKeyEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const Color  key_color     = get_color (params, "key_color",     Color{0.0f, 1.0f, 0.0f, 1.0f});
    const float  similarity    = std::max(0.001f, get_scalar(params, "similarity",     0.40f));
    const float  softness      = std::max(0.001f, get_scalar(params, "softness",       0.10f));
    const float  clip_black    = clamp01(get_scalar(params, "clip_black",    0.00f));
    const float  clip_white    = clamp01(get_scalar(params, "clip_white",    1.00f));
    const float  matte_erosion = std::max(0.0f,  get_scalar(params, "matte_erosion", 0.0f));
    const float  matte_feather = std::max(0.0f,  get_scalar(params, "matte_feather", 0.0f));
    const float  spill         = clamp01(get_scalar(params, "spill",         0.50f));
    const float  despill_bias  = clamp01(get_scalar(params, "despill_bias",  0.50f));

    const uint32_t W = input.width();
    const uint32_t H = input.height();
    if (W == 0 || H == 0) return input;

    // -----------------------------------------------------------------------
    // Helper: convert linear RGB → YCbCr (Rec.601 coefficients work well
    // for screen matching even in a linear working space).
    // -----------------------------------------------------------------------
    auto rgb_to_ycbcr = [](float r, float g, float b, float& Y, float& Cb, float& Cr) {
        Y  =  0.2126f * r + 0.7152f * g + 0.0722f * b;
        Cb = -0.1146f * r - 0.3854f * g + 0.5000f * b;
        Cr =  0.5000f * r - 0.4542f * g - 0.0458f * b;
    };

    // Key colour in YCbCr
    float kY, kCb, kCr;
    rgb_to_ycbcr(key_color.r, key_color.g, key_color.b, kY, kCb, kCr);

    // Max chrominance extent of key (used to normalise distances)
    const float key_chroma = std::sqrt(kCb * kCb + kCr * kCr);
    const float inv_key_chroma = (key_chroma > 1e-5f) ? (1.0f / key_chroma) : 0.0f;

    // Detect dominant key channel for despill (independent of CbCr step)
    const bool green_key = key_color.g >= key_color.r && key_color.g >= key_color.b;
    const bool blue_key  = !green_key && (key_color.b >= key_color.r);

    // -----------------------------------------------------------------------
    // Stage 1 + 2 + 5 – per-pixel: compute raw matte & despill colour.
    // We store raw matte in a separate flat buffer so we can post-process it.
    // -----------------------------------------------------------------------
    std::vector<float> raw_alpha(static_cast<size_t>(W) * H, 1.0f);
    SurfaceRGBA despilled(W, H);
    despilled.clear(Color::transparent());

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            Color px = input.get_pixel(x, y);
            if (px.a <= 0.0f) {
                raw_alpha[y * W + x] = 0.0f;
                despilled.set_pixel(x, y, px);
                continue;
            }

            // --- CbCr distance ---
            float pY, pCb, pCr;
            rgb_to_ycbcr(px.r, px.g, px.b, pY, pCb, pCr);

            // Project pixel chrominance onto key chrominance axis
            float dot   = pCb * kCb + pCr * kCr;   // dot(pixel_chroma, key_chroma)
            float proj  = dot * inv_key_chroma;       // signed distance along key axis

            // Perpendicular residual (colour difference to key plane)
            float perp  = std::sqrt(std::max(0.0f,
                (pCb - kCb * proj * inv_key_chroma) * (pCb - kCb * proj * inv_key_chroma) +
                (pCr - kCr * proj * inv_key_chroma) * (pCr - kCr * proj * inv_key_chroma)));

            // Normalised match: 0 = exact key, 1 = far from key
            float match = std::clamp(proj * inv_key_chroma, 0.0f, 1.0f); // how much screen present
            (void)perp; // reserved for future edge-detection pass

            // Smoothstep within softness band
            float edge0 = similarity - softness * 0.5f;
            float edge1 = similarity + softness * 0.5f;
            float t = std::clamp((match - edge0) / std::max(edge1 - edge0, 1e-5f), 0.0f, 1.0f);
            float alpha_raw = t * t * (3.0f - 2.0f * t); // smoothstep

            // --- Clip black / white ---
            if (clip_black > 0.0f || clip_white < 1.0f) {
                float range = std::max(clip_white - clip_black, 1e-5f);
                alpha_raw = std::clamp((alpha_raw - clip_black) / range, 0.0f, 1.0f);
            }

            raw_alpha[y * W + x] = alpha_raw * px.a;

            // --- Despill (Stage 5) – always applied; blended by strength ---
            Color despilled_px = px;
            if (spill > 0.0f && match < 1.0f) {  // only desaturate pixels near key
                float despill_strength = spill * (1.0f - match);
                if (green_key) {
                    // Neutral target: weighted average of R and B.
                    // despill_bias: 0 = cold (favour blue), 1 = warm (favour red)
                    float neutral = lerp(despilled_px.b, despilled_px.r, despill_bias);
                    despilled_px.g = lerp(despilled_px.g, std::min(despilled_px.g, neutral), despill_strength);
                } else if (blue_key) {
                    float neutral = lerp(despilled_px.r, despilled_px.g, despill_bias);
                    despilled_px.b = lerp(despilled_px.b, std::min(despilled_px.b, neutral), despill_strength);
                }
            }

            despilled.set_pixel(x, y, despilled_px);
        }
    }

    // -----------------------------------------------------------------------
    // Stage 3 – Matte erosion: min-filter on the alpha buffer.
    // -----------------------------------------------------------------------
    if (matte_erosion >= 1.0f) {
        const int r = std::max(1, static_cast<int>(std::round(matte_erosion)));
        std::vector<float> eroded(raw_alpha.size());
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                float mn = 1.0f;
                for (int dy = -r; dy <= r; ++dy) {
                    for (int dx = -r; dx <= r; ++dx) {
                        int nx = static_cast<int>(x) + dx;
                        int ny = static_cast<int>(y) + dy;
                        if (nx < 0 || ny < 0 || nx >= (int)W || ny >= (int)H) continue;
                        mn = std::min(mn, raw_alpha[(uint32_t)ny * W + (uint32_t)nx]);
                    }
                }
                eroded[y * W + x] = mn;
            }
        }
        raw_alpha = std::move(eroded);
    }

    // -----------------------------------------------------------------------
    // Stage 4 – Matte feather: gaussian blur on alpha buffer.
    // -----------------------------------------------------------------------
    if (matte_feather >= 0.5f) {
        const auto k = gaussian_kernel(matte_feather);
        // Build premultiplied alpha-only buffer for the separable blur
        std::vector<PremultipliedPixel> alpha_px(raw_alpha.size());
        for (size_t i = 0; i < raw_alpha.size(); ++i)
            alpha_px[i] = {0.0f, 0.0f, 0.0f, raw_alpha[i]};
        auto blurred = convolve_v(convolve_h(alpha_px, W, H, k), W, H, k);
        for (size_t i = 0; i < raw_alpha.size(); ++i)
            raw_alpha[i] = std::clamp(blurred[i].a, 0.0f, 1.0f);
    }

    // -----------------------------------------------------------------------
    // Compose final output: despilled colour × post-processed matte alpha.
    // -----------------------------------------------------------------------
    SurfaceRGBA result(W, H);
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            Color px = despilled.get_pixel(x, y);
            px.a = raw_alpha[y * W + x];
            result.set_pixel(x, y, px);
        }
    }

    // -----------------------------------------------------------------------
    // A2 – Garbage Matte: punch a hole in the matte wherever the garbage mask
    // is opaque.  A white pixel in the garbage matte = "exclude this area".
    // params.aux_surfaces["garbage_matte"] is optional.
    // -----------------------------------------------------------------------
    const auto gm_it = params.aux_surfaces.find("garbage_matte");
    if (gm_it != params.aux_surfaces.end() && gm_it->second) {
        const SurfaceRGBA& gm = *gm_it->second;
        const uint32_t gw = std::min(W, gm.width());
        const uint32_t gh = std::min(H, gm.height());
        for (uint32_t y = 0; y < gh; ++y) {
            for (uint32_t x = 0; x < gw; ++x) {
                float excl = gm.get_pixel(x, y).a;   // 1 = exclude, 0 = keep
                if (excl > 0.0f) {
                    Color px = result.get_pixel(x, y);
                    px.a *= std::max(0.0f, 1.0f - excl);
                    result.set_pixel(x, y, px);
                }
            }
        }
    }

    return result;
}

// =============================================================================
// A3 – Light Wrap
// =============================================================================
// Params:
//   amount       (scalar) – blend strength               [0..1], default 0.5
//   radius       (scalar) – dilation + blur radius (px)  [0..∞), default 15.0
//   aux_surfaces["background"] – the composited plate below this layer.
// =============================================================================
SurfaceRGBA LightWrapEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const auto bg_it = params.aux_surfaces.find("background");
    if (bg_it == params.aux_surfaces.end() || !bg_it->second) {
        return input;   // no background: pass through
    }
    const SurfaceRGBA& bg = *bg_it->second;

    const float amount = clamp01(get_scalar(params, "amount", 0.5f));
    const float radius = std::max(1.0f, get_scalar(params, "radius", 15.0f));
    if (amount <= 0.0f) return input;

    const uint32_t W = input.width();
    const uint32_t H = input.height();

    // 1. Build a silhouette of the foreground (alpha channel).
    // 2. Dilate the silhouette by `radius` pixels (max-filter approximation via blur).
    // 3. Multiply the dilated silhouette with the blurred background – this gives
    //    the "light from plate bleeding onto the edge of the subject".
    // 4. Blend that bleeding onto the foreground, weighted by (1 - fg_alpha) * amount.

    // Blur the background to simulate soft environmental light
    SurfaceRGBA bg_blurred = blur_surface(bg, radius);

    // Build dilated alpha: blur the alpha then threshold at >0
    std::vector<PremultipliedPixel> alpha_buf(static_cast<size_t>(W) * H);
    for (uint32_t y = 0; y < H; ++y)
        for (uint32_t x = 0; x < W; ++x)
            alpha_buf[y * W + x] = {0.0f, 0.0f, 0.0f, input.get_pixel(x, y).a};

    const auto k = gaussian_kernel(radius);
    auto dilated_buf = convolve_v(convolve_h(alpha_buf, W, H, k), W, H, k);

    SurfaceRGBA result(W, H);
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            Color fg = input.get_pixel(x, y);
            float dilated_alpha = std::clamp(dilated_buf[y * W + x].a, 0.0f, 1.0f);

            // Sample the plate at this position (clamp to bg dimensions)
            uint32_t bx = std::min(x, bg_blurred.width()  - 1);
            uint32_t by = std::min(y, bg_blurred.height() - 1);
            Color plate = bg_blurred.get_pixel(bx, by);

            // How much wrap: proportional to dilated alpha outside the solid fg core
            float wrap_weight = dilated_alpha * (1.0f - fg.a) * amount;
            Color out{
                fg.r + plate.r * wrap_weight,
                fg.g + plate.g * wrap_weight,
                fg.b + plate.b * wrap_weight,
                fg.a + wrap_weight * plate.a
            };
            out.r = std::clamp(out.r, 0.0f, 1.0f);
            out.g = std::clamp(out.g, 0.0f, 1.0f);
            out.b = std::clamp(out.b, 0.0f, 1.0f);
            out.a = std::clamp(out.a, 0.0f, 1.0f);
            result.set_pixel(x, y, out);
        }
    }
    return result;
}

// =============================================================================
// A5 – Matte Refinement
// =============================================================================
// Params:
//   choke        (scalar) – erode alpha N px (positive = shrink)    default 0
//   spread       (scalar) – dilate alpha N px (positive = expand)   default 0
//   denoise      (scalar) – median-like alpha smooth strength [0..1] default 0
//   temporal     (scalar) – temporal alpha blend with prev frame     default 0
//   clip_black   (scalar) – crush matte blacks                       default 0
//   clip_white   (scalar) – crush matte whites                       default 1
//   strings["layer_id"]  – ID for the temporal cache bucket
// =============================================================================

// Static members
std::unordered_map<std::string, std::vector<float>> MatteRefinementEffect::s_prev_alpha;
std::mutex MatteRefinementEffect::s_cache_mutex;

void MatteRefinementEffect::set_previous_frame(const std::string& id, std::vector<float> alpha) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    s_prev_alpha[id] = std::move(alpha);
}

const std::vector<float>* MatteRefinementEffect::get_previous_frame(const std::string& id) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    auto it = s_prev_alpha.find(id);
    return (it != s_prev_alpha.end()) ? &it->second : nullptr;
}

SurfaceRGBA MatteRefinementEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const int   choke      = static_cast<int>(std::round(get_scalar(params, "choke",     0.0f)));
    const int   spread     = static_cast<int>(std::round(get_scalar(params, "spread",    0.0f)));
    const float denoise    = clamp01(get_scalar(params, "denoise",   0.0f));
    const float temporal   = clamp01(get_scalar(params, "temporal",  0.0f));
    const float clip_black = clamp01(get_scalar(params, "clip_black",0.0f));
    const float clip_white = clamp01(get_scalar(params, "clip_white",1.0f));
    const auto layer_id_it = params.strings.find("layer_id");
    const std::string layer_id = (layer_id_it != params.strings.end()) ? layer_id_it->second : "";

    const uint32_t W = input.width();
    const uint32_t H = input.height();

    // Extract alpha buffer
    std::vector<float> alpha(static_cast<size_t>(W) * H);
    for (uint32_t y = 0; y < H; ++y)
        for (uint32_t x = 0; x < W; ++x)
            alpha[y * W + x] = input.get_pixel(x, y).a;

    // --- Clip ---
    if (clip_black > 0.0f || clip_white < 1.0f) {
        float range = std::max(clip_white - clip_black, 1e-5f);
        for (float& a : alpha)
            a = std::clamp((a - clip_black) / range, 0.0f, 1.0f);
    }

    // --- Choke (erosion, min-filter) ---
    if (choke > 0) {
        std::vector<float> tmp(alpha.size());
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                float mn = 1.0f;
                for (int dy = -choke; dy <= choke; ++dy)
                    for (int dx = -choke; dx <= choke; ++dx) {
                        int nx = (int)x + dx, ny = (int)y + dy;
                        if (nx < 0 || ny < 0 || nx >= (int)W || ny >= (int)H) continue;
                        mn = std::min(mn, alpha[(uint32_t)ny * W + (uint32_t)nx]);
                    }
                tmp[y * W + x] = mn;
            }
        }
        alpha = std::move(tmp);
    }

    // --- Spread (dilation, max-filter) ---
    if (spread > 0) {
        std::vector<float> tmp(alpha.size());
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                float mx = 0.0f;
                for (int dy = -spread; dy <= spread; ++dy)
                    for (int dx = -spread; dx <= spread; ++dx) {
                        int nx = (int)x + dx, ny = (int)y + dy;
                        if (nx < 0 || ny < 0 || nx >= (int)W || ny >= (int)H) continue;
                        mx = std::max(mx, alpha[(uint32_t)ny * W + (uint32_t)nx]);
                    }
                tmp[y * W + x] = mx;
            }
        }
        alpha = std::move(tmp);
    }

    // --- Denoise: 3×3 median-like smooth (box mean weighted toward current) ---
    if (denoise > 0.0f) {
        std::vector<float> tmp(alpha.size());
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                float sum = 0.0f; int cnt = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = (int)x + dx, ny = (int)y + dy;
                        if (nx < 0 || ny < 0 || nx >= (int)W || ny >= (int)H) continue;
                        sum += alpha[(uint32_t)ny * W + (uint32_t)nx];
                        ++cnt;
                    }
                float avg = (cnt > 0) ? sum / cnt : alpha[y * W + x];
                tmp[y * W + x] = lerp(alpha[y * W + x], avg, denoise);
            }
        }
        alpha = std::move(tmp);
    }

    // --- Temporal smoothing ---
    if (temporal > 0.0f && !layer_id.empty()) {
        const std::vector<float>* prev = get_previous_frame(layer_id);
        if (prev && prev->size() == alpha.size()) {
            for (size_t i = 0; i < alpha.size(); ++i)
                alpha[i] = lerp((*prev)[i], alpha[i], 1.0f - temporal);
        }
    }

    // Store for next frame
    if (!layer_id.empty()) {
        set_previous_frame(layer_id, alpha);
    }

    // Compose result: original RGB, refined alpha
    SurfaceRGBA result(W, H);
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            Color px = input.get_pixel(x, y);
            px.a = alpha[y * W + x];
            result.set_pixel(x, y, px);
        }
    }
    return result;
}

// =============================================================================
// A4 – Vector Blur
// =============================================================================
// Params:
//   samples      (scalar) – number of samples along velocity vector [1..32], default 8
//   scale        (scalar) – multiply velocity magnitude             [0..∞), default 1.0
//   aux_surfaces["motion_vectors"] – RG channels = XY velocity in pixels/frame
//                                    (encoded as linear float; 0 = zero velocity).
//   Fallback if no motion vector surface: radial zoom blur from centre.
// =============================================================================
SurfaceRGBA VectorBlurEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const int   samples = std::max(1, static_cast<int>(get_scalar(params, "samples", 8.0f)));
    const float scale   = std::max(0.0f, get_scalar(params, "scale", 1.0f));

    const uint32_t W = input.width();
    const uint32_t H = input.height();
    if (W == 0 || H == 0 || scale <= 0.0f) return input;

    const auto mv_it = params.aux_surfaces.find("motion_vectors");
    const SurfaceRGBA* mv_surface = (mv_it != params.aux_surfaces.end()) ? mv_it->second : nullptr;

    SurfaceRGBA result(W, H);
    const float fw = static_cast<float>(W);
    const float fh = static_cast<float>(H);
    const float inv_samples = 1.0f / static_cast<float>(samples);

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            float vx, vy;
            if (mv_surface) {
                // Motion vector surface: R = vx, G = vy (both in [-1..1] mapped to pixel space)
                // Convention: R=0.5,G=0.5 means zero velocity
                uint32_t mx = std::min(x, mv_surface->width()  - 1);
                uint32_t my = std::min(y, mv_surface->height() - 1);
                Color mv = mv_surface->get_pixel(mx, my);
                vx = (mv.r - 0.5f) * 2.0f * fw * scale;
                vy = (mv.g - 0.5f) * 2.0f * fh * scale;
            } else {
                // Fallback: radial from centre, velocity proportional to distance
                float cx = (static_cast<float>(x) - fw * 0.5f) / fw;
                float cy = (static_cast<float>(y) - fh * 0.5f) / fh;
                float dist = std::sqrt(cx * cx + cy * cy);
                vx = cx * dist * scale * fw * 0.1f;
                vy = cy * dist * scale * fh * 0.1f;
            }

            // Accumulate samples along the velocity vector
            float ar = 0, ag = 0, ab = 0, aa = 0;
            for (int s = 0; s < samples; ++s) {
                float t = (samples > 1) ? (static_cast<float>(s) / (samples - 1) - 0.5f) : 0.0f;
                float sx = static_cast<float>(x) + vx * t;
                float sy = static_cast<float>(y) + vy * t;
                // Bilinear sample
                Color c = sample_texture_bilinear(input,
                    std::clamp(sx / fw, 0.0f, 1.0f),
                    std::clamp(sy / fh, 0.0f, 1.0f),
                    Color{1,1,1,1});
                ar += c.r; ag += c.g; ab += c.b; aa += c.a;
            }
            result.set_pixel(x, y, Color{ar * inv_samples, ag * inv_samples,
                                         ab * inv_samples, aa * inv_samples});
        }
    }
    return result;
}

} // namespace tachyon::renderer2d
