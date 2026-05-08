#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/glitch.h"
#include "tachyon/renderer2d/effects/core/effect_common.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectDescriptor> get_distortion_effect_descriptors() {
    std::vector<EffectDescriptor> descriptors;

    // Chromatic Aberration
    descriptors.push_back({
        "tachyon.effect.distort.chromatic_aberration",
        {"tachyon.effect.distort.chromatic_aberration", "Chromatic Aberration", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"strength", "Strength", "RGB channel offset", 2.0, 0.0, 20.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ChromaticAberrationEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Vignette
    descriptors.push_back({
        "tachyon.effect.distort.vignette",
        {"tachyon.effect.distort.vignette", "Vignette", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Vignette size", 0.5, 0.0, 1.0},
            {"softness", "Softness", "Edge feathering", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Darkening amount", 1.0, 0.0, 1.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            VignetteEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Displacement Map
    descriptors.push_back({
        "tachyon.effect.distort.displacement",
        {"tachyon.effect.distort.displacement", "Displacement Map", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"map_layer", "Map Layer", "Layer ID for displacement", ""},
            {"strength_x", "Strength X", "Horizontal displacement", 10.0, -500.0, 500.0},
            {"strength_y", "Strength Y", "Vertical displacement", 10.0, -500.0, 500.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            DisplacementMapEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Warp Stabilizer
    descriptors.push_back({
        "tachyon.effect.distort.warp_stabilizer",
        {"tachyon.effect.distort.warp_stabilizer", "Warp Stabilizer", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"crop_percent", "Crop %", "Auto-crop amount", 10.0, 0.0, 100.0},
            {"smoothing", "Smoothing", "Stabilization strength", 50.0, 0.0, 100.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            WarpStabilizerEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Digital Glitch
    EffectDescriptor glitch;
    glitch.id = "tachyon.effect.distort.glitch";
    glitch.metadata = {"tachyon.effect.distort.glitch", "Digital Glitch", "Digital artifacting and displacement.", "effect.distort", {"glitch", "artifact"}};
    glitch.schema = registry::ParameterSchema({
            {"intensity", "Intensity", "Glitch strength", 0.5, 0.0, 1.0},
            {"seed", "Seed", "Random seed", 0.0, 0.0, 9999.0},
            {"rgb_shift_px", "RGB Shift (px)", "Color separation amount", 2.0, 0.0, 20.0},
            {"block_size", "Block Size", "Size of glitch blocks", 16.0, 4.0, 128.0},
            {"scan_line_chance", "Scan Line Chance", "Probability of scan line artifacts", 0.1, 0.0, 1.0}
        });
    glitch.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
             GlitchEffect effect;
             effect.intensity = get_scalar(params, "intensity", 0.5f);
             effect.seed = static_cast<uint64_t>(get_scalar(params, "seed", 0.0f));
             effect.rgb_shift_px = get_scalar(params, "rgb_shift_px", 2.0f);
             effect.block_size = get_scalar(params, "block_size", 16.0f);
             effect.scan_line_chance = get_scalar(params, "scan_line_chance", 0.1f);
             output = input;
        };
    descriptors.push_back(std::move(glitch));

    return descriptors;
}

std::vector<presets::EffectPresetSpec> get_distortion_effect_preset_specs() {
    std::vector<presets::EffectPresetSpec> specs;

    // Chromatic Aberration Preset
    specs.push_back({
        "tachyon.effect.distort.chromatic_aberration",
        {"tachyon.effect.distort.chromatic_aberration", "Chromatic Aberration", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"strength", "Strength", "RGB channel offset", 2.0, 0.0, 20.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.distort.chromatic_aberration";
            effect.scalars["strength"] = p.get_or<double>("strength", 2.0);
            return effect;
        }
    });

    // Vignette Preset
    specs.push_back({
        "tachyon.effect.distort.vignette",
        {"tachyon.effect.distort.vignette", "Vignette", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Vignette size", 0.5, 0.0, 1.0},
            {"softness", "Softness", "Edge feathering", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Darkening amount", 1.0, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.distort.vignette";
            effect.scalars["radius"] = p.get_or<double>("radius", 0.5);
            effect.scalars["softness"] = p.get_or<double>("softness", 0.5);
            effect.scalars["strength"] = p.get_or<double>("strength", 1.0);
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::renderer2d
