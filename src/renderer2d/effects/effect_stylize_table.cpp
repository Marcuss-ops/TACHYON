#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/effect_descriptor.h"

namespace tachyon::renderer2d {

namespace {

static const std::array<EffectBuiltinSpec, 4> kStylizeEffects = {{
    {
        "tachyon.effect.stylize.drop_shadow",
        "Drop Shadow",
        "stylize",
        "Adds a soft shadow behind the layer.",
        {"shadow", "drop", "depth"},
        registry::ParameterSchema({
            {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
            {"offset_x", "Offset X", "Horizontal shift", 4.0},
            {"offset_y", "Offset Y", "Vertical shift", 4.0},
            {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
        }),
        make_effect_factory<DropShadowEffect>()
    },
    {
        "tachyon.effect.stylize.glow",
        "Glow",
        "stylize",
        "Adds a luminous halo to bright areas.",
        {"glow", "bloom", "light"},
        registry::ParameterSchema({
            {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
            {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
            {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
        }),
        make_effect_factory<GlowEffect>()
    },
    {
        "tachyon.effect.stylize.motion_blur_2d",
        "Motion Blur 2D",
        "stylize",
        "Linear directional blur for motion simulation.",
        {"motion", "blur", "speed"},
        registry::ParameterSchema({
            {"samples", "Samples", "Number of blur samples", 8.0, 1.0, 64.0},
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0},
            {"shutter_angle", "Shutter Angle", "Simulated shutter (180 = half frame)", 180.0, 0.0, 720.0}
        }),
        make_effect_factory<MotionBlur2DEffect>()
    },
    {
        "tachyon.effect.keying.chroma_key",
        "Chroma Key",
        "keying",
        "Removes a specific color from the image.",
        {"keying", "chroma", "green-screen"},
        registry::ParameterSchema({
            {"key_color", "Key Color", "Color to remove", ColorSpec{0, 255, 0, 255}},
            {"threshold", "Threshold", "Sensitivity", 0.1, 0.0, 1.0},
            {"softness", "Softness", "Edge softness", 0.1, 0.0, 1.0}
        }),
        make_effect_factory<ChromaKeyEffect>()
    }
}};

} // namespace

void register_stylize_effects(EffectRegistry& registry) {
    for (const auto& spec : kStylizeEffects) {
        register_effect_from_spec(registry, spec);
    }
}

} // namespace tachyon::renderer2d
