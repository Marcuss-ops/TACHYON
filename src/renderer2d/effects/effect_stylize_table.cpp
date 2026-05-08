#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::renderer2d {

std::vector<EffectDescriptor> get_stylize_effect_descriptors() {
    std::vector<EffectDescriptor> descriptors;

    // Drop Shadow
    descriptors.push_back({
        "tachyon.effect.stylize.drop_shadow",
        {"tachyon.effect.stylize.drop_shadow", "Drop Shadow", "Adds a soft shadow behind the layer.", "effect.stylize", {"shadow", "drop", "depth"}},
        registry::ParameterSchema({
            {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
            {"offset_x", "Offset X", "Horizontal shift", 4.0},
            {"offset_y", "Offset Y", "Vertical shift", 4.0},
            {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            DropShadowEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Glow
    descriptors.push_back({
        "tachyon.effect.stylize.glow",
        {"tachyon.effect.stylize.glow", "Glow", "Adds a luminous halo to bright areas.", "effect.stylize", {"glow", "bloom", "light"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
            {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
            {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            GlowEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Motion Blur 2D
    descriptors.push_back({
        "tachyon.effect.stylize.motion_blur_2d",
        {"tachyon.effect.stylize.motion_blur_2d", "Motion Blur 2D", "Linear directional blur for motion simulation.", "effect.stylize", {"motion", "blur", "speed"}},
        registry::ParameterSchema({
            {"samples", "Samples", "Number of blur samples", 8.0, 1.0, 64.0},
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0},
            {"shutter_angle", "Shutter Angle", "Simulated shutter (180 = half frame)", 180.0, 0.0, 720.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            MotionBlur2DEffect effect;
            output = effect.apply(input, params);
        }
    });

    // Keying (Chroma Key)
    descriptors.push_back({
        "tachyon.effect.keying.chroma_key",
        {"tachyon.effect.keying.chroma_key", "Chroma Key", "Removes a specific color from the image.", "effect.keying", {"keying", "chroma", "green-screen"}},
        registry::ParameterSchema({
            {"key_color", "Key Color", "Color to remove", ColorSpec{0, 255, 0, 255}},
            {"threshold", "Threshold", "Sensitivity", 0.1, 0.0, 1.0},
            {"softness", "Softness", "Edge softness", 0.1, 0.0, 1.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            ChromaKeyEffect effect;
            output = effect.apply(input, params);
        }
    });

    return descriptors;
}

std::vector<presets::EffectPresetSpec> get_stylize_effect_preset_specs() {
    std::vector<presets::EffectPresetSpec> specs;

    // Drop Shadow Preset
    specs.push_back({
        "tachyon.effect.stylize.drop_shadow",
        {"tachyon.effect.stylize.drop_shadow", "Drop Shadow", "Adds a soft shadow behind the layer.", "effect.stylize", {"shadow", "drop", "depth"}},
        registry::ParameterSchema({
            {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
            {"offset_x", "Offset X", "Horizontal shift", 4.0},
            {"offset_y", "Offset Y", "Vertical shift", 4.0},
            {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.stylize.drop_shadow";
            effect.scalars["blur_radius"] = p.get_or<double>("blur_radius", 4.0);
            effect.scalars["offset_x"] = p.get_or<double>("offset_x", 4.0);
            effect.scalars["offset_y"] = p.get_or<double>("offset_y", 4.0);
            effect.colors["shadow_color"] = p.get_or<ColorSpec>("shadow_color", ColorSpec{0, 0, 0, 160});
            return effect;
        }
    });

    // Glow Preset
    specs.push_back({
        "tachyon.effect.stylize.glow",
        {"tachyon.effect.stylize.glow", "Glow", "Adds a luminous halo to bright areas.", "effect.stylize", {"glow", "bloom", "light"}},
        registry::ParameterSchema({
            {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
            {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
            {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.stylize.glow";
            effect.scalars["radius"] = p.get_or<double>("radius", 4.0);
            effect.scalars["strength"] = p.get_or<double>("strength", 1.0);
            effect.scalars["threshold"] = p.get_or<double>("threshold", 0.0);
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::renderer2d
