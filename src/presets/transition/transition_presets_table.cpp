#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

void register_glsl(TransitionPresetRegistry& registry, std::string id, std::string name, std::string transition_id) {
    registry.register_spec({
        id,
        {id, name, "Professional GLSL-based transition effect.", "transition", {"glsl", "cinematic"}},
        [transition_id](const TransitionParams& p) {
            LayerTransitionSpec spec;
            spec.type = transition_id;
            spec.transition_id = transition_id;
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
    register_spec({
        "tachyon.transition.none",
        {"tachyon.transition.none", "None", "No transition effect", "transition", {"utility"}},
        [](const TransitionParams&) {
            LayerTransitionSpec spec;
            spec.type = "tachyon.transition.none";
            spec.transition_id = "tachyon.transition.none";
            return spec;
        }
    });

    // 1. Classic Crossfade
    register_glsl(*this, "tachyon.transition.crossfade", "Crossfade", "tachyon.transition.crossfade");

    // 2. Circle Iris
    register_glsl(*this, "tachyon.transition.circle_iris", "Circle Iris", "tachyon.transition.circle_iris");

    // 3. Pixelate
    register_glsl(*this, "tachyon.transition.pixelate", "Pixelate", "tachyon.transition.pixelate");

    // 4. Zoom Blur
    register_glsl(*this, "tachyon.transition.zoom_blur", "Zoom Blur", "tachyon.transition.zoom_blur");

    // 5. Luma Dissolve
    register_glsl(*this, "tachyon.transition.luma_dissolve", "Luma Dissolve", "tachyon.transition.luma_dissolve");

    // 6. Directional Blur Wipe
    register_glsl(*this, "tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", "tachyon.transition.directional_blur_wipe");

    // 7. RGB Split
    register_glsl(*this, "tachyon.transition.rgb_split", "RGB Split", "tachyon.transition.rgb_split");

    // 8. Push Left (Geometric)
    register_glsl(*this, "tachyon.transition.push_left", "Push Left", "tachyon.transition.push_left");

    // 9. Wipe Linear
    register_glsl(*this, "tachyon.transition.wipe_linear", "Wipe Linear", "tachyon.transition.wipe_linear");

    // 10. Spin
    register_glsl(*this, "tachyon.transition.spin", "Spin", "tachyon.transition.spin");

    // Extra: Glitch Slice
    register_glsl(*this, "tachyon.transition.glitch_slice", "Glitch Slice", "tachyon.transition.glitch_slice");
    
    // Cinematic
    register_glsl(*this, "tachyon.transition.film_burn", "Film Burn", "tachyon.transition.film_burn");
    register_glsl(*this, "tachyon.transition.light_leak", "Light Leak", "tachyon.transition.light_leak");
}

} // namespace tachyon::presets
