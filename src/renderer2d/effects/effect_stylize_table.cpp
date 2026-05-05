#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

void register_stylize_effects(EffectRegistry& registry) {
    // Drop Shadow
    EffectDescriptor drop_shadow_desc;
    drop_shadow_desc.id = "tachyon.effect.stylize.drop_shadow";
    drop_shadow_desc.metadata = {
        drop_shadow_desc.id,
        "Drop Shadow",
        "Adds a soft shadow behind the layer.",
        "effect.stylize",
        {"shadow", "drop", "depth"}
    };
    drop_shadow_desc.schema = registry::ParameterSchema({
        {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
        {"offset_x", "Offset X", "Horizontal shift", 4.0},
        {"offset_y", "Offset Y", "Vertical shift", 4.0},
        {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
    });
    drop_shadow_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        DropShadowEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(drop_shadow_desc));

    // Glow
    EffectDescriptor glow_desc;
    glow_desc.id = "tachyon.effect.stylize.glow";
    glow_desc.metadata = {
        glow_desc.id,
        "Glow",
        "Adds a luminous halo to bright areas.",
        "effect.stylize",
        {"glow", "bloom", "light"}
    };
    glow_desc.schema = registry::ParameterSchema({
        {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
        {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
        {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
    });
    glow_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        GlowEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(glow_desc));

    // Motion Blur 2D
    EffectDescriptor motion_blur_desc;
    motion_blur_desc.id = "tachyon.effect.stylize.motion_blur_2d";
    motion_blur_desc.metadata = {
        motion_blur_desc.id,
        "Motion Blur 2D",
        "Linear directional blur for motion simulation.",
        "effect.stylize",
        {"motion", "blur", "speed"}
    };
    motion_blur_desc.schema = registry::ParameterSchema({
        {"samples", "Samples", "Number of blur samples", 8.0, 1.0, 64.0},
        {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
        {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0},
        {"shutter_angle", "Shutter Angle", "Simulated shutter (180 = half frame)", 180.0, 0.0, 720.0}
    });
    motion_blur_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        MotionBlur2DEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(motion_blur_desc));

    // Keying (Chroma Key)
    EffectDescriptor chroma_key_desc;
    chroma_key_desc.id = "tachyon.effect.keying.chroma_key";
    chroma_key_desc.metadata = {
        chroma_key_desc.id,
        "Chroma Key",
        "Removes a specific color from the image.",
        "effect.keying",
        {"keying", "chroma", "green-screen"}
    };
    chroma_key_desc.schema = registry::ParameterSchema({
        {"key_color", "Key Color", "Color to remove", ColorSpec{0, 255, 0, 255}},
        {"threshold", "Threshold", "Sensitivity", 0.1, 0.0, 1.0},
        {"softness", "Softness", "Edge softness", 0.1, 0.0, 1.0}
    });
    chroma_key_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        ChromaKeyEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(chroma_key_desc));
}

} // namespace tachyon::renderer2d
