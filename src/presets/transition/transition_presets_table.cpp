#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

void register_builtin(
    const std::string& id,
    const std::string& name,
    TransitionKind kind,
    const std::vector<std::string>& tags = {"cinematic"}) 
{
    (void)kind;
    (void)tags;

    TransitionPresetSpec spec;
    spec.id = id;
    spec.metadata = registry::RegistryMetadata{
        id,
        name,
        name + " transition preset",
        "transition",
        tags
    };
    spec.schema = registry::ParameterSchema({
        {"duration", "Duration", "Transition duration", 0.4, 0.0, 10.0},
        {"delay", "Delay", "Transition delay", 0.0, 0.0, 10.0}
    });
    spec.factory = [kind, id](const registry::ParameterBag& p) {
        return TransitionPresetRegistry::build_spec_from_params(p, kind, id);
    };
    TransitionPresetRegistry::instance().register_spec(std::move(spec));
}

} // namespace

void TransitionPresetRegistry::load_builtins() {
    // Utility
    register_builtin("tachyon.transition.none", "None", TransitionKind::None, {"utility"});

    // Canonical presets backed by verified library assets
    register_builtin("tachyon.transition.crossfade", "Crossfade", TransitionKind::Fade);
    register_builtin("tachyon.transition.slide_up", "Slide Up", TransitionKind::Slide);
    register_builtin("tachyon.transition.swipe_left", "Swipe Left", TransitionKind::Wipe);
    register_builtin("tachyon.transition.fade_to_black", "Fade to Black", TransitionKind::Fade);
    register_builtin("tachyon.transition.wipe_linear", "Wipe Linear", TransitionKind::Wipe);
    register_builtin("tachyon.transition.wipe_angular", "Wipe Angular", TransitionKind::Wipe);
    register_builtin("tachyon.transition.push_left", "Push Left", TransitionKind::Slide);
    register_builtin("tachyon.transition.slide_easing", "Slide Easing", TransitionKind::Slide);
    register_builtin("tachyon.transition.zoom_in", "Zoom In", TransitionKind::Zoom);
    register_builtin("tachyon.transition.zoom_blur", "Zoom Blur", TransitionKind::Zoom);
    register_builtin("tachyon.transition.spin", "Spin", TransitionKind::Flip);
    register_builtin("tachyon.transition.circle_iris", "Circle Iris", TransitionKind::Wipe);
    register_builtin("tachyon.transition.pixelate", "Pixelate", TransitionKind::Custom);
    register_builtin("tachyon.transition.glitch_slice", "Glitch Slice", TransitionKind::Custom);
    register_builtin("tachyon.transition.rgb_split", "RGB Split", TransitionKind::Custom);
    register_builtin("tachyon.transition.luma_dissolve", "Luma Dissolve", TransitionKind::Dissolve);
    register_builtin("tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", TransitionKind::Wipe);
    register_builtin("tachyon.transition.kaleidoscope", "Kaleidoscope", TransitionKind::Custom);
    register_builtin("tachyon.transition.ripple", "Ripple", TransitionKind::Custom);
}

} // namespace tachyon::presets
