#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/effect_common.h"

namespace tachyon::renderer2d {

SurfaceRGBA DropShadowEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float blur_radius = get_scalar(params, "blur_radius", 4.0f);
    const int offset_x     = static_cast<int>(get_scalar(params, "offset_x", 4.0f));
    const int offset_y     = static_cast<int>(get_scalar(params, "offset_y", 4.0f));
    const Color shadow_color = get_color(params, "shadow_color", Color{0.0f, 0.0f, 0.0f, 160.0f / 255.0f});

    SurfaceRGBA shadow = blur_alpha_mask(input, blur_radius);
    for (std::uint32_t y = 0; y < shadow.height(); ++y) {
        for (std::uint32_t x = 0; x < shadow.width(); ++x) {
            const float alpha = shadow.get_pixel(x, y).a;
            if (alpha <= 0.0f) continue;
            const float sa = alpha * shadow_color.a;
            shadow.set_pixel(x, y, Color{shadow_color.r, shadow_color.g, shadow_color.b, sa});
        }
    }

    SurfaceRGBA out(input.width(), input.height());
    composite_with_offset(out, shadow, offset_x, offset_y);
    composite_with_offset(out, input, 0, 0);
    return out;
}

SurfaceRGBA GlowEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float radius   = get_scalar(params, "radius", get_scalar(params, "blur_radius", 4.0f));
    const float strength = std::max(0.0f, get_scalar(params, "strength", 1.0f));
    const float threshold = std::max(0.0f, get_scalar(params, "threshold", 0.0f));
    
    if (input.width() == 0U || input.height() == 0U) return input;

    // Create a thresholded version for the glow source
    SurfaceRGBA source = transform_surface(input, [&](Color px) {
        const float l = luminance(px);
        if (l < threshold) return Color::transparent();
        return px;
    });

    const SurfaceRGBA blurred = blur_surface(source, radius);
    SurfaceRGBA out(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const Color base = input.get_pixel(x, y);
            const Color glow = blurred.get_pixel(x, y);
            
            // Screen blend for better highlights
            const float r = 1.0f - (1.0f - base.r) * (1.0f - glow.r * strength);
            const float g = 1.0f - (1.0f - base.g) * (1.0f - glow.g * strength);
            const float b = 1.0f - (1.0f - base.b) * (1.0f - glow.b * strength);
            
            out.set_pixel(x, y, Color{
                std::clamp(r, 0.0f, 1.0f),
                std::clamp(g, 0.0f, 1.0f),
                std::clamp(b, 0.0f, 1.0f),
                base.a
            });
        }
    }
    return out;
}

SurfaceRGBA ChromaticAberrationEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float amount = get_scalar(params, "amount", 2.0f);
    const float x_dir = get_scalar(params, "x_direction", 1.0f);
    const float y_dir = get_scalar(params, "y_direction", 0.0f);

    const int ox = static_cast<int>(std::lround(amount * x_dir));
    const int oy = static_cast<int>(std::lround(amount * y_dir));

    SurfaceRGBA out(input.width(), input.height());
    out.clear(Color::transparent());

    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const int rx = std::clamp(static_cast<int>(x) - ox, 0, static_cast<int>(input.width()) - 1);
            const int ry = std::clamp(static_cast<int>(y) - oy, 0, static_cast<int>(input.height()) - 1);
            const int bx = std::clamp(static_cast<int>(x) + ox, 0, static_cast<int>(input.width()) - 1);
            const int by = std::clamp(static_cast<int>(y) + oy, 0, static_cast<int>(input.height()) - 1);

            const Color r_px = input.get_pixel(static_cast<std::uint32_t>(rx), static_cast<std::uint32_t>(ry));
            const Color g_px = input.get_pixel(x, y);
            const Color b_px = input.get_pixel(static_cast<std::uint32_t>(bx), static_cast<std::uint32_t>(by));

            out.set_pixel(x, y, Color{ r_px.r, g_px.g, b_px.b, g_px.a });
        }
    }
    return out;
}

SurfaceRGBA VignetteEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float amount = clamp01(get_scalar(params, "amount", 0.5f));
    const float size = std::max(0.01f, get_scalar(params, "size", 0.5f));
    const float centerX = get_scalar(params, "center_x", 0.5f) * static_cast<float>(input.width());
    const float centerY = get_scalar(params, "center_y", 0.5f) * static_cast<float>(input.height());
    const float maxDist = std::sqrt(centerX * centerX + centerY * centerY);

    SurfaceRGBA out(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const float dx = static_cast<float>(x) - centerX;
            const float dy = static_cast<float>(y) - centerY;
            const float dist = std::sqrt(dx * dx + dy * dy);
            const float factor = std::clamp((dist / maxDist) / size, 0.0f, 1.0f);
            const float v = 1.0f - factor * amount;

            const Color px = input.get_pixel(x, y);
            out.set_pixel(x, y, Color{px.r * v, px.g * v, px.b * v, px.a});
        }
    }
    return out;
}

SurfaceRGBA DisplacementMapEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const auto it = params.textures.find("displacement_source");
    if (it == params.textures.end() || !it->second) return input;
    
    const SurfaceRGBA& source = *it->second;
    const float max_displace_x = get_scalar(params, "max_displacement_x", 10.0f);
    const float max_displace_y = get_scalar(params, "max_displacement_y", 10.0f);
    
    SurfaceRGBA output(input.width(), input.height());
    
    for (uint32_t y = 0; y < input.height(); ++y) {
        for (uint32_t x = 0; x < input.width(); ++x) {
            float u = static_cast<float>(x) / input.width();
            float v = static_cast<float>(y) / input.height();
            
            uint32_t sx = static_cast<uint32_t>(std::clamp(u * source.width(), 0.0f, static_cast<float>(source.width() - 1)));
            uint32_t sy = static_cast<uint32_t>(std::clamp(v * source.height(), 0.0f, static_cast<float>(source.height() - 1)));
            
            Color source_px = source.get_pixel(sx, sy);
            
            float dx = (source_px.r - 0.5f) * max_displace_x;
            float dy = (source_px.g - 0.5f) * max_displace_y;
            
            float target_u = (static_cast<float>(x) + dx) / input.width();
            float target_v = (static_cast<float>(y) + dy) / input.height();
            
            output.set_pixel(x, y, sample_texture_bilinear(input, target_u, target_v, Color::white()));
        }
    }
    return output;
}

} // namespace tachyon::renderer2d

