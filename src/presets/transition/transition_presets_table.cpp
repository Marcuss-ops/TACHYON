#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

LayerTransitionSpec make_transition_spec(const registry::ParameterBag& p, TransitionKind kind, const std::string& transition_id) {
    LayerTransitionSpec spec;
    spec.kind = kind;
    spec.type = transition_id;
    spec.transition_id = transition_id;
    spec.duration = p.get_or<double>("duration", 0.4);
    spec.easing = static_cast<animation::EasingPreset>(p.get_or<int>("easing", static_cast<int>(animation::EasingPreset::EaseOut)));
    spec.delay = p.get_or<double>("delay", 0.0);
    spec.direction = p.get_or<std::string>("direction", "none");
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
        {
            id, name, std::move(description), "transition", {"glsl", "cinematic"},
            1, registry::Stability::Stable,
            {{"image", "video", "solid"}, {"gpu"}}
        },
        {},
        [kind, transition_id = std::move(transition_id)](const registry::ParameterBag& p) {
            return make_transition_spec(p, kind, transition_id);
        }
    });
}

} // namespace

void TransitionPresetRegistry::load_builtins() {
    register_spec({
        "tachyon.transition.none",
        {"tachyon.transition.none", "None", "No transition effect", "transition", {"utility"}, 1, registry::Stability::Stable, {{"all"}, {}}},
        {},
        [](const registry::ParameterBag&) {
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
    register_glsl(*this, "tachyon.transition.kaleidoscope", "Kaleidoscope", "Dynamic kaleidoscope transition", TransitionKind::Custom, "tachyon.transition.kaleidoscope");
    register_glsl(*this, "tachyon.transition.ripple", "Ripple", "Water ripple transition", TransitionKind::Custom, "tachyon.transition.ripple");

    // Cinematic Light Leaks and Film Effects
    register_glsl(*this, "tachyon.transition.light_leak", "Light Leak", "High-quality evolving cinematic light leak", TransitionKind::Fade, "tachyon.transition.light_leak");
    register_glsl(*this, "tachyon.transition.film_burn", "Film Burn", "Fiery red-orange film burn", TransitionKind::Fade, "tachyon.transition.film_burn");

    // Premium Light Leaks
    register_glsl(*this, "tachyon.transition.lightleak.soft_warm_edge", "Soft Warm Edge Leak", "Premium warm edge light leak", TransitionKind::Fade, "tachyon.transition.lightleak.soft_warm_edge");
    register_glsl(*this, "tachyon.transition.lightleak.golden_sweep", "Golden Sweep", "Soft golden cinematic sweep", TransitionKind::Fade, "tachyon.transition.lightleak.golden_sweep");
    register_glsl(*this, "tachyon.transition.lightleak.creamy_white", "Creamy White Leak", "Soft warm white memory leak", TransitionKind::Fade, "tachyon.transition.lightleak.creamy_white");
    register_glsl(*this, "tachyon.transition.lightleak.dusty_archive", "Dusty Archive Leak", "Warm archival light leak with subtle grain", TransitionKind::Fade, "tachyon.transition.lightleak.dusty_archive");
    register_glsl(*this, "tachyon.transition.lightleak.lens_flare_pass", "Subtle Lens Flare Pass", "Thin premium lens flare sweep", TransitionKind::Fade, "tachyon.transition.lightleak.lens_flare_pass");
    register_glsl(*this, "tachyon.transition.lightleak.amber_sweep", "Amber Sweep", "Warm multi-blob amber light leak sweeping left to right", TransitionKind::Fade, "tachyon.transition.lightleak.amber_sweep");
}

} // namespace tachyon::presets
