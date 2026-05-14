#include "tachyon/presets/effects/effect_specs.h"
#include <vector>

namespace tachyon::presets {

std::vector<EffectKindSpec> get_distortion_effect_kind_specs() {
    std::vector<EffectKindSpec> specs;

    // Chromatic Aberration
    specs.push_back({
        "tachyon.effect.distort.chromatic_aberration",
        {"tachyon.effect.distort.chromatic_aberration", "Chromatic Aberration", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"strength", "Strength", "RGB channel offset", 2.0, 0.0, 20.0}
        })
    });

    // Vignette
    specs.push_back({
        "tachyon.effect.distort.vignette",
        {"tachyon.effect.distort.vignette", "Vignette", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Vignette size", 0.5, 0.0, 1.0},
            {"softness", "Softness", "Edge feathering", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Darkening amount", 1.0, 0.0, 1.0}
        })
    });

    // Displacement Map
    specs.push_back({
        "tachyon.effect.distort.displacement",
        {"tachyon.effect.distort.displacement", "Displacement Map", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"map_layer", "Map Layer", "Layer ID for displacement", ""},
            {"strength_x", "Strength X", "Horizontal displacement", 10.0, -500.0, 500.0},
            {"strength_y", "Strength Y", "Vertical displacement", 10.0, -500.0, 500.0}
        })
    });

    // Warp Stabilizer
    specs.push_back({
        "tachyon.effect.distort.warp_stabilizer",
        {"tachyon.effect.distort.warp_stabilizer", "Warp Stabilizer", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"crop_percent", "Crop %", "Auto-crop amount", 10.0, 0.0, 100.0},
            {"smoothing", "Smoothing", "Stabilization strength", 50.0, 0.0, 100.0}
        })
    });

    // Digital Glitch
    specs.push_back({
        "tachyon.effect.distort.glitch",
        {"tachyon.effect.distort.glitch", "Digital Glitch", "Digital artifacting and displacement.", "effect.distort", {"glitch", "artifact"}},
        registry::ParameterSchema({
            {"intensity", "Intensity", "Glitch strength", 0.5, 0.0, 1.0},
            {"seed", "Seed", "Random seed", 0.0, 0.0, 9999.0},
            {"rgb_shift_px", "RGB Shift (px)", "Color separation amount", 2.0, 0.0, 20.0},
            {"block_size", "Block Size", "Size of glitch blocks", 16.0, 4.0, 128.0},
            {"scan_line_chance", "Scan Line Chance", "Probability of scan line artifacts", 0.1, 0.0, 1.0}
        })
    });

    return specs;
}

std::vector<EffectPresetSpec> get_distortion_effect_preset_specs() {
    std::vector<EffectPresetSpec> specs;

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
            effect.params["strength"] = p.get_or<double>("strength", 2.0);
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
            effect.params["radius"] = p.get_or<double>("radius", 0.5);
            effect.params["softness"] = p.get_or<double>("softness", 0.5);
            effect.params["strength"] = p.get_or<double>("strength", 1.0);
            return effect;
        }
    });

    // Displacement Map Preset
    specs.push_back({
        "tachyon.effect.distort.displacement",
        {"tachyon.effect.distort.displacement", "Displacement Map", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"map_layer", "Map Layer", "Layer ID for displacement", ""},
            {"strength_x", "Strength X", "Horizontal displacement", 10.0, -500.0, 500.0},
            {"strength_y", "Strength Y", "Vertical displacement", 10.0, -500.0, 500.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.distort.displacement";
            effect.params["map_layer"] = p.get_or<std::string>("map_layer", "");
            effect.params["strength_x"] = p.get_or<double>("strength_x", 10.0);
            effect.params["strength_y"] = p.get_or<double>("strength_y", 10.0);
            return effect;
        }
    });

    // Warp Stabilizer Preset
    specs.push_back({
        "tachyon.effect.distort.warp_stabilizer",
        {"tachyon.effect.distort.warp_stabilizer", "Warp Stabilizer", "Professional distortion effect.", "effect.distort", {"distort"}},
        registry::ParameterSchema({
            {"crop_percent", "Crop %", "Auto-crop amount", 10.0, 0.0, 100.0},
            {"smoothing", "Smoothing", "Stabilization strength", 50.0, 0.0, 100.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.distort.warp_stabilizer";
            effect.params["crop_percent"] = p.get_or<double>("crop_percent", 10.0);
            effect.params["smoothing"] = p.get_or<double>("smoothing", 50.0);
            return effect;
        }
    });

    // Digital Glitch Preset
    specs.push_back({
        "tachyon.effect.distort.glitch",
        {"tachyon.effect.distort.glitch", "Digital Glitch", "Digital artifacting and displacement.", "effect.distort", {"glitch", "artifact"}},
        registry::ParameterSchema({
            {"intensity", "Intensity", "Glitch strength", 0.5, 0.0, 1.0},
            {"seed", "Seed", "Random seed", 0.0, 0.0, 9999.0},
            {"rgb_shift_px", "RGB Shift (px)", "Color separation amount", 2.0, 0.0, 20.0},
            {"block_size", "Block Size", "Size of glitch blocks", 16.0, 4.0, 128.0},
            {"scan_line_chance", "Scan Line Chance", "Probability of scan line artifacts", 0.1, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.distort.glitch";
            effect.params["intensity"] = p.get_or<double>("intensity", 0.5);
            effect.params["seed"] = p.get_or<double>("seed", 0.0);
            effect.params["rgb_shift_px"] = p.get_or<double>("rgb_shift_px", 2.0);
            effect.params["block_size"] = p.get_or<double>("block_size", 16.0);
            effect.params["scan_line_chance"] = p.get_or<double>("scan_line_chance", 0.1);
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::presets
