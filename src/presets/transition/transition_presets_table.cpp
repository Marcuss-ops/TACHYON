#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"

namespace tachyon::presets {

namespace {

void register_builtin(
    const std::string& id,
    const std::string& name,
    TransitionKind kind,
    TransitionBackend backend,
    const std::string& renderer_effect_id,
    const std::vector<std::string>& tags = {"cinematic"}) 
{
    TransitionDescriptor desc;
    desc.id = id;
    desc.display_name = name;
    desc.description = name + " transition effect";
    desc.kind = kind;
    desc.backend = backend;
    desc.renderer_effect_id = renderer_effect_id;
    desc.tags = tags;
    desc.category = "transition";
    desc.supports_gpu = (backend == TransitionBackend::Glsl);
    desc.supports_cpu = (backend == TransitionBackend::CpuPixel || backend == TransitionBackend::StateOnly);
    
    register_transition_descriptor(desc);
}

} // namespace

void TransitionPresetRegistry::load_builtins() {
    // Utility
    register_builtin("tachyon.transition.none", "None", TransitionKind::None, TransitionBackend::StateOnly, "none", {"utility"});

    // Canonical presets backed by verified library assets
    register_builtin("tachyon.transition.crossfade", "Crossfade", TransitionKind::Fade, TransitionBackend::Glsl, "crossfade");
    register_builtin("tachyon.transition.slide_up", "Slide Up", TransitionKind::Slide, TransitionBackend::Glsl, "slide_up");
    register_builtin("tachyon.transition.swipe_left", "Swipe Left", TransitionKind::Wipe, TransitionBackend::Glsl, "swipe_left");
    register_builtin("tachyon.transition.fade_to_black", "Fade to Black", TransitionKind::Fade, TransitionBackend::Glsl, "fade_to_black");
    register_builtin("tachyon.transition.wipe_linear", "Wipe Linear", TransitionKind::Wipe, TransitionBackend::Glsl, "wipe_linear");
    register_builtin("tachyon.transition.wipe_angular", "Wipe Angular", TransitionKind::Wipe, TransitionBackend::Glsl, "wipe_angular");
    register_builtin("tachyon.transition.push_left", "Push Left", TransitionKind::Slide, TransitionBackend::Glsl, "push_left");
    register_builtin("tachyon.transition.slide_easing", "Slide Easing", TransitionKind::Slide, TransitionBackend::Glsl, "slide_easing");
    register_builtin("tachyon.transition.zoom_in", "Zoom In", TransitionKind::Zoom, TransitionBackend::Glsl, "zoom_in");
    register_builtin("tachyon.transition.zoom_blur", "Zoom Blur", TransitionKind::Zoom, TransitionBackend::Glsl, "zoom_blur");
    register_builtin("tachyon.transition.spin", "Spin", TransitionKind::Flip, TransitionBackend::Glsl, "spin");
    register_builtin("tachyon.transition.circle_iris", "Circle Iris", TransitionKind::Wipe, TransitionBackend::Glsl, "circle_iris");
    register_builtin("tachyon.transition.pixelate", "Pixelate", TransitionKind::Custom, TransitionBackend::Glsl, "pixelate");
    register_builtin("tachyon.transition.glitch_slice", "Glitch Slice", TransitionKind::Custom, TransitionBackend::Glsl, "glitch_slice");
    register_builtin("tachyon.transition.rgb_split", "RGB Split", TransitionKind::Custom, TransitionBackend::Glsl, "rgb_split");
    register_builtin("tachyon.transition.luma_dissolve", "Luma Dissolve", TransitionKind::Dissolve, TransitionBackend::Glsl, "luma_dissolve");
    register_builtin("tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", TransitionKind::Wipe, TransitionBackend::Glsl, "directional_blur_wipe");
    register_builtin("tachyon.transition.kaleidoscope", "Kaleidoscope", TransitionKind::Custom, TransitionBackend::Glsl, "kaleidoscope");
    register_builtin("tachyon.transition.ripple", "Ripple", TransitionKind::Custom, TransitionBackend::Glsl, "ripple");
}

} // namespace tachyon::presets
