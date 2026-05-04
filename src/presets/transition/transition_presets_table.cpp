#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

void register_glsl(TransitionPresetRegistry& registry, std::string id, std::string name, std::string shader_id) {
    registry.register_preset({
        std::move(id),
        std::move(name),
        "Professional GLSL-based transition effect.",
        [shader_id](const TransitionParams& p) {
            LayerTransitionSpec spec;
            // Use transition_id to tell the renderer which shader/function to use
            spec.transition_id = shader_id;
            spec.duration = p.duration;
            spec.easing = p.easing;
            spec.delay = p.delay;
            spec.direction = p.direction;
            return spec;
        }
    });
}

} // namespace

void TransitionPresetRegistry::load_builtins() {
    register_preset({
        "none", "None", "No transition effect",
        [](const TransitionParams& p) {
            LayerTransitionSpec spec;
            spec.type = "none";
            return spec;
        }
    });

    // 1. Classic Crossfade
    register_glsl(*this, "crossfade", "Crossfade", "fade");

    // 2. Circle Iris
    register_glsl(*this, "circle_iris", "Circle Iris", "circle_iris");

    // 3. Pixelate
    register_glsl(*this, "pixelate", "Pixelate", "pixelate");

    // 4. Zoom Blur
    register_glsl(*this, "zoom_blur", "Zoom Blur", "zoom_blur");

    // 5. Luma Dissolve
    register_glsl(*this, "luma_dissolve", "Luma Dissolve", "luma_dissolve");

    // 6. Directional Blur Wipe
    register_glsl(*this, "directional_blur_wipe", "Directional Blur Wipe", "directional_blur_wipe");

    // 7. RGB Split
    register_glsl(*this, "rgb_split", "RGB Split", "rgb_split");

    // 8. Push Left (Geometric)
    register_glsl(*this, "push_left", "Push Left", "push_left");

    // 9. Wipe Linear
    register_glsl(*this, "wipe_linear", "Wipe Linear", "wipe_linear");

    // 10. Spin
    register_glsl(*this, "spin", "Spin", "spin");

    // Extra: Glitch Slice
    register_glsl(*this, "glitch_slice", "Glitch Slice", "glitch_slice");
    
    // Cinematic
    register_glsl(*this, "film_burn", "Film Burn", "film_burn");
    register_glsl(*this, "light_leak", "Light Leak", "light_leak");
}

} // namespace tachyon::presets
