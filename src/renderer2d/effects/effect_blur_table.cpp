#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

std::vector<EffectDescriptor> get_blur_effect_descriptors() {
    std::vector<EffectDescriptor> descriptors;

    // Gaussian Blur
    descriptors.push_back({
        "tachyon.effect.blur.gaussian",
        {"tachyon.effect.blur.gaussian", "Gaussian Blur", "Professional blur effect.", "effect.blur", {"blur"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Blur radius in pixels", 4.0, 0.0, 100.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GaussianBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Directional Blur
    descriptors.push_back({
        "tachyon.effect.blur.directional",
        {"tachyon.effect.blur.directional", "Directional Blur", "Professional blur effect.", "effect.blur", {"blur"}},
        registry::ParameterSchema({
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            DirectionalBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Radial Blur
    descriptors.push_back({
        "tachyon.effect.blur.radial",
        {"tachyon.effect.blur.radial", "Radial Blur", "Professional blur effect.", "effect.blur", {"blur"}},
        registry::ParameterSchema({
            {"center_x", "Center X", "Radial center X (normalized)", 0.5, 0.0, 1.0},
            {"center_y", "Center Y", "Radial center Y (normalized)", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Blur strength", 10.0, 0.0, 100.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            RadialBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Vector Blur
    descriptors.push_back({
        "tachyon.effect.blur.vector",
        {"tachyon.effect.blur.vector", "Vector Blur", "Professional blur effect.", "effect.blur", {"blur"}},
        registry::ParameterSchema({
            {"angle", "Angle", "Blur angle in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur distance in pixels", 10.0, 0.0, 500.0},
            {"samples", "Samples", "Number of samples", 8.0, 1.0, 64.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            VectorBlurEffect effect;
            output = effect.apply(input, params);
        }
    });

    return descriptors;
}

} // namespace tachyon::renderer2d
