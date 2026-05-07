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
        {id, name, "Professional distortion effect.", "effect." + category, {"distort", category}},
        std::move(schema),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        }
    });
}
 
} // namespace
 
void register_distortion_effects(EffectRegistry& registry) {
    register_builtin<ChromaticAberrationEffect>(registry, "tachyon.effect.distort.chromatic_aberration", "Chromatic Aberration", "distort",
        registry::ParameterSchema({
            {"strength", "Strength", "RGB channel offset", 2.0, 0.0, 20.0}
        }));
    register_builtin<VignetteEffect>(registry, "tachyon.effect.distort.vignette", "Vignette", "distort",
        registry::ParameterSchema({
            {"radius", "Radius", "Vignette size", 0.5, 0.0, 1.0},
            {"softness", "Softness", "Edge feathering", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Darkening amount", 1.0, 0.0, 1.0}
        }));
    register_builtin<DisplacementMapEffect>(registry, "tachyon.effect.distort.displacement", "Displacement Map", "distort",
        registry::ParameterSchema({
            {"map_layer", "Map Layer", "Layer ID for displacement", ""},
            {"strength_x", "Strength X", "Horizontal displacement", 10.0, -500.0, 500.0},
            {"strength_y", "Strength Y", "Vertical displacement", 10.0, -500.0, 500.0}
        }));
    register_builtin<WarpStabilizerEffect>(registry, "tachyon.effect.distort.warp_stabilizer", "Warp Stabilizer", "distort",
        registry::ParameterSchema({
            {"crop_percent", "Crop %", "Auto-crop amount", 10.0, 0.0, 100.0},
            {"smoothing", "Smoothing", "Stabilization strength", 50.0, 0.0, 100.0}
        }));
}
 
void register_glitch_effects(EffectRegistry& registry) {
    registry.register_spec({
        "tachyon.effect.distort.glitch",
        {"tachyon.effect.distort.glitch", "Digital Glitch", "Digital artifacting and displacement.", "effect.distort", {"glitch", "artifact"}},
        registry::ParameterSchema({
            {"intensity", "Intensity", "Glitch strength", 0.5, 0.0, 1.0},
            {"seed", "Seed", "Random seed", 0.0, 0.0, 9999.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            (void)params;
            output = input;
        }
    });
}

} // namespace tachyon::renderer2d
