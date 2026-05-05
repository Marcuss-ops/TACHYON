#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

LayerTransitionSpec make_transition_spec(const TransitionParams& p, TransitionKind kind, const std::string& transition_id) {
    LayerTransitionSpec spec;
    spec.kind = kind;
    spec.type = transition_id;
    spec.transition_id = transition_id;
    spec.duration = p.duration;
    spec.easing = p.easing;
    spec.delay = p.delay;
    spec.direction = p.direction;
    return spec;
}

void register_glsl(TransitionPresetRegistry& registry,
                   std::string id,
                   std::string name,
                   std::string description,
                   TransitionKind kind,
                   std::string transition_id) {
    registry.register_spec({
        id,
        {id, name, std::move(description), "transition", {"glsl", "cinematic"}},
        [kind, transition_id = std::move(transition_id)](const TransitionParams& p) {
            return make_transition_spec(p, kind, transition_id);
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
            spec.kind = TransitionKind::None;
            return spec;
        }
    });

    // Canonical presets backed by verified library assets
    register_glsl(*this, "tachyon.transition.crossfade", "Crossfade", "Simple crossfade transition", TransitionKind::Fade, "tachyon.transition.crossfade");
    register_glsl(*this, "tachyon.transition.slide_up", "Slide Up", "Vertical slide transition", TransitionKind::Slide, "tachyon.transition.slide_up");
    register_glsl(*this, "tachyon.transition.swipe_left", "Swipe Left", "Swipe left reveal transition", TransitionKind::Wipe, "tachyon.transition.swipe_left");
    register_glsl(*this, "tachyon.transition.fade_to_black", "Fade to Black", "Crossfade through black", TransitionKind::Fade, "tachyon.transition.fade_to_black");
    register_glsl(*this, "tachyon.transition.wipe_linear", "Wipe Linear", "Simple left-to-right wipe", TransitionKind::Wipe, "tachyon.transition.wipe_linear");
    register_glsl(*this, "tachyon.transition.wipe_angular", "Wipe Angular", "Angular wipe around center", TransitionKind::Wipe, "tachyon.transition.wipe_angular");
    register_glsl(*this, "tachyon.transition.push_left", "Push Left", "Push image to the left", TransitionKind::Slide, "tachyon.transition.push_left");
    register_glsl(*this, "tachyon.transition.slide_easing", "Slide Easing", "Slide with easing", TransitionKind::Slide, "tachyon.transition.slide_easing");
    register_glsl(*this, "tachyon.transition.zoom_in", "Zoom In", "Zoom into target", TransitionKind::Zoom, "tachyon.transition.zoom_in");
    register_glsl(*this, "tachyon.transition.zoom_blur", "Zoom Blur", "Zoom with motion blur", TransitionKind::Zoom, "tachyon.transition.zoom_blur");
    register_glsl(*this, "tachyon.transition.spin", "Spin", "Spin rotation", TransitionKind::Flip, "tachyon.transition.spin");
    register_glsl(*this, "tachyon.transition.circle_iris", "Circle Iris", "Circular iris opener", TransitionKind::Wipe, "tachyon.transition.circle_iris");
    register_glsl(*this, "tachyon.transition.pixelate", "Pixelate", "Pixelation transition", TransitionKind::Custom, "tachyon.transition.pixelate");
    register_glsl(*this, "tachyon.transition.glitch_slice", "Glitch Slice", "Glitchy slice effect", TransitionKind::Custom, "tachyon.transition.glitch_slice");
    register_glsl(*this, "tachyon.transition.rgb_split", "RGB Split", "Color channel split", TransitionKind::Custom, "tachyon.transition.rgb_split");
    register_glsl(*this, "tachyon.transition.luma_dissolve", "Luma Dissolve", "Luminance-based dissolve", TransitionKind::Dissolve, "tachyon.transition.luma_dissolve");
    register_glsl(*this, "tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", "Blur wipe with direction", TransitionKind::Wipe, "tachyon.transition.directional_blur_wipe");
}

} // namespace tachyon::presets
