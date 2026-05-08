#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/glitch.h"
#include "tachyon/renderer2d/effects/core/effect_common.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_distortion_effect_implementations() {
    std::vector<EffectImplementation> implementations;

    // Chromatic Aberration
    implementations.push_back({
        "tachyon.effect.distort.chromatic_aberration",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ChromaticAberrationEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Vignette
    implementations.push_back({
        "tachyon.effect.distort.vignette",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            VignetteEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Displacement Map
    implementations.push_back({
        "tachyon.effect.distort.displacement",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            DisplacementMapEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Warp Stabilizer
    implementations.push_back({
        "tachyon.effect.distort.warp_stabilizer",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            WarpStabilizerEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Digital Glitch
    implementations.push_back({
        "tachyon.effect.distort.glitch",
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GlitchEffect effect;
            effect.intensity = get_scalar(params, "intensity", 0.5f);
            effect.seed = static_cast<uint64_t>(get_scalar(params, "seed", 0.0f));
            effect.rgb_shift_px = get_scalar(params, "rgb_shift_px", 2.0f);
            effect.block_size = get_scalar(params, "block_size", 16.0f);
            effect.scan_line_chance = get_scalar(params, "scan_line_chance", 0.1f);
            output = input;
        }
    });

    return implementations;
}

} // namespace tachyon::renderer2d
