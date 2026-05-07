#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/blur/blur_effects.h"

#include <array>

namespace tachyon::renderer2d {

namespace {

static const std::array<EffectBuiltinSpec, 4> kBlurEffects = {{
    {
        "tachyon.effect.blur.gaussian",
        "Gaussian Blur",
        "blur",
        "Blur radius in pixels",
        {"blur", "soften"},
        registry::ParameterSchema({
            {"radius", "Radius", "Blur radius in pixels", 4.0, 0.0, 100.0}
        }),
        make_effect_factory<GaussianBlurEffect>()
    },
    {
        "tachyon.effect.blur.directional",
        "Directional Blur",
        "blur",
        "Blur direction in degrees",
        {"blur", "direction"},
        registry::ParameterSchema({
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0}
        }),
        make_effect_factory<DirectionalBlurEffect>()
    },
    {
        "tachyon.effect.blur.radial",
        "Radial Blur",
        "blur",
        "Radial blur effect.",
        {"blur", "radial"},
        registry::ParameterSchema({
            {"center_x", "Center X", "Radial center X (normalized)", 0.5, 0.0, 1.0},
            {"center_y", "Center Y", "Radial center Y (normalized)", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Blur strength", 10.0, 0.0, 100.0}
        }),
        make_effect_factory<RadialBlurEffect>()
    },
    {
        "tachyon.effect.blur.vector",
        "Vector Blur",
        "blur",
        "Vector blur effect.",
        {"blur", "vector"},
        registry::ParameterSchema({
            {"angle", "Angle", "Blur angle in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur distance in pixels", 10.0, 0.0, 500.0},
            {"samples", "Samples", "Number of samples", 8.0, 1.0, 64.0}
        }),
        make_effect_factory<VectorBlurEffect>()
    }
}};

} // namespace

void register_blur_effects(EffectRegistry& registry) {
    for (const auto& spec : kBlurEffects) {
        register_effect_from_spec(registry, spec);
    }
}

} // namespace tachyon::renderer2d
