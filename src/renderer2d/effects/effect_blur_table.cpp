#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

namespace {
 
template <typename T>
void register_builtin(
    EffectRegistry& registry,
    std::string id,
    std::string name,
    std::string category,
    registry::ParameterSchema schema) {
    registry.register_spec({
        id,
        {id, name, "Professional blur effect.", "effect." + category, {"blur", category}},
        std::move(schema),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        }
    });
}
 
} // namespace
 
void register_blur_effects(EffectRegistry& registry) {
    register_builtin<GaussianBlurEffect>(registry, "tachyon.effect.blur.gaussian", "Gaussian Blur", "blur",
        registry::ParameterSchema({
            {"radius", "Radius", "Blur radius in pixels", 4.0, 0.0, 100.0}
        }));
    register_builtin<DirectionalBlurEffect>(registry, "tachyon.effect.blur.directional", "Directional Blur", "blur",
        registry::ParameterSchema({
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0}
        }));
    register_builtin<RadialBlurEffect>(registry, "tachyon.effect.blur.radial", "Radial Blur", "blur",
        registry::ParameterSchema({
            {"center_x", "Center X", "Radial center X (normalized)", 0.5, 0.0, 1.0},
            {"center_y", "Center Y", "Radial center Y (normalized)", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Blur strength", 10.0, 0.0, 100.0}
        }));
    register_builtin<VectorBlurEffect>(registry, "tachyon.effect.blur.vector", "Vector Blur", "blur",
        registry::ParameterSchema({
            {"angle", "Angle", "Blur angle in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur distance in pixels", 10.0, 0.0, 500.0},
            {"samples", "Samples", "Number of samples", 8.0, 1.0, 64.0}
        }));
}

} // namespace tachyon::renderer2d
