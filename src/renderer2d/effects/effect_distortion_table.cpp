#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/effect_descriptor.h"

namespace tachyon::renderer2d {

namespace {

constexpr std::array<EffectBuiltinSpec, 5> kDistortionEffects = {{
    {
        "tachyon.effect.distort.chromatic_aberration",
        "Chromatic Aberration",
        "distort",
        {},
        {},
        registry::ParameterSchema({
            {"strength", "Strength", "RGB channel offset", 2.0, 0.0, 20.0}
        }),
        make_effect_factory<ChromaticAberrationEffect>()
    },
    {
        "tachyon.effect.distort.vignette",
        "Vignette",
        "distort",
        {},
        {},
        registry::ParameterSchema({
            {"radius", "Radius", "Vignette size", 0.5, 0.0, 1.0},
            {"softness", "Softness", "Edge feathering", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Darkening amount", 1.0, 0.0, 1.0}
        }),
        make_effect_factory<VignetteEffect>()
    },
    {
        "tachyon.effect.distort.displacement",
        "Displacement Map",
        "distort",
        {},
        {},
        registry::ParameterSchema({
            {"map_layer", "Map Layer", "Layer ID for displacement", ""},
            {"strength_x", "Strength X", "Horizontal displacement", 10.0, -500.0, 500.0},
            {"strength_y", "Strength Y", "Vertical displacement", 10.0, -500.0, 500.0}
        }),
        make_effect_factory<DisplacementMapEffect>()
    },
    {
        "tachyon.effect.distort.warp_stabilizer",
        "Warp Stabilizer",
        "distort",
        {},
        {},
        registry::ParameterSchema({
            {"crop_percent", "Crop %", "Auto-crop amount", 10.0, 0.0, 100.0},
            {"smoothing", "Smoothing", "Stabilization strength", 50.0, 0.0, 100.0}
        }),
        make_effect_factory<WarpStabilizerEffect>()
    },
    {
        "tachyon.effect.distort.glitch",
        "Digital Glitch",
        "distort",
        "Digital artifacting and displacement.",
        {"glitch", "artifact"},
        registry::ParameterSchema({
            {"intensity", "Intensity", "Glitch strength", 0.5, 0.0, 1.0},
            {"seed", "Seed", "Random seed", 0.0, 0.0, 9999.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output,
           const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            (void)params;
            output = input;
        }
    }
}};

} // namespace

void register_distortion_effects(EffectRegistry& registry) {
    for (const auto& spec : kDistortionEffects) {
        register_effect_from_spec(registry, spec);
    }
}

void register_glitch_effects(EffectRegistry& registry) {
    // Glitch is now registered via distortion table; kept for API compatibility
}

} // namespace tachyon::renderer2d
