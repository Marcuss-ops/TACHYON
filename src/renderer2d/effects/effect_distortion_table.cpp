#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

namespace {

template <typename T>
void register_builtin(
    EffectRegistry& registry,
    std::string id,
    std::string name,
    std::string category) {
    registry.register_spec({
        id,
        {id, name, "Professional distortion effect.", "effect." + category, {"distort", category}},
        {}, // ParameterSchema
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        }
    });
}

} // namespace

void register_distortion_effects(EffectRegistry& registry) {
    register_builtin<ChromaticAberrationEffect>(registry, "tachyon.effect.distort.chromatic_aberration", "Chromatic Aberration", "distort");
    register_builtin<VignetteEffect>(registry, "tachyon.effect.distort.vignette", "Vignette", "distort");
    register_builtin<DisplacementMapEffect>(registry, "tachyon.effect.distort.displacement", "Displacement Map", "distort");
    register_builtin<WarpStabilizerEffect>(registry, "tachyon.effect.distort.warp_stabilizer", "Warp Stabilizer", "distort");
}

void register_glitch_effects(EffectRegistry& registry) {
    // We can group glitch under distortion or separate it
    registry.register_spec({
        "tachyon.effect.distort.glitch",
        {"tachyon.effect.distort.glitch", "Digital Glitch", "Digital artifacting and displacement.", "effect.distort", {"glitch", "artifact"}},
        {}, // ParameterSchema
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            (void)params;
            output = input;
        }
    });
}

} // namespace tachyon::renderer2d
