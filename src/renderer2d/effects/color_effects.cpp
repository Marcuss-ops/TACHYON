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

} // namespace tachyon::renderer2d