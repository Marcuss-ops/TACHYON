#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/ids/builtin_ids.h"

namespace tachyon::presets {

namespace {
 
void register_builtin(
    TransitionPresetRegistry& registry,
    const std::string& id,
    const std::string& name,
    const std::string& canonical_id,
    const std::vector<std::string>& tags = {"cinematic"}) 
{
    TransitionPresetSpec spec;
    spec.id = id;
    spec.metadata = registry::RegistryMetadata{
        id,
        name,
        name + " transition preset",
        "transition",
        tags,
        1,
        registry::Stability::Stable,
        {}
    };
    spec.schema = registry::ParameterSchema({
        {"duration", "Duration", "Transition duration", 0.4, 0.0, 10.0},
        {"delay", "Delay", "Transition delay", 0.0, 0.0, 10.0}
    });
    spec.factory = [canonical_id](const registry::ParameterBag& p) {
        // Presets are now pure aliases to canonical transition IDs
        return TransitionPresetRegistry::build_spec_from_params(p, TransitionKind::Custom, canonical_id);
    };
    registry.register_spec(std::move(spec));
}
 
} // namespace

void TransitionPresetRegistry::load_builtins() {
    using namespace tachyon::ids::transition;

    // Utility
    register_builtin(*this, "tachyon.transition.none", "None", "none", {"utility"});

    // Canonical presets mapping to verified builtin IDs
    register_builtin(*this, "tachyon.transition.crossfade", "Crossfade", std::string(crossfade));
    register_builtin(*this, "tachyon.transition.slide_up", "Slide Up", std::string(slide_up));
    register_builtin(*this, "tachyon.transition.swipe_left", "Swipe Left", std::string(swipe_left));
    register_builtin(*this, "tachyon.transition.fade_to_black", "Fade to Black", std::string(fade_to_black));
    register_builtin(*this, "tachyon.transition.wipe_linear", "Wipe Linear", std::string(wipe_linear));
    register_builtin(*this, "tachyon.transition.wipe_angular", "Wipe Angular", std::string(wipe_angular));
    register_builtin(*this, "tachyon.transition.push_left", "Push Left", std::string(push_left));
    register_builtin(*this, "tachyon.transition.slide_easing", "Slide Easing", std::string(slide_easing));
    register_builtin(*this, "tachyon.transition.zoom_in", "Zoom In", std::string(zoom_in));
    register_builtin(*this, "tachyon.transition.zoom_blur", "Zoom Blur", std::string(zoom_blur));
    register_builtin(*this, "tachyon.transition.spin", "Spin", std::string(spin));
    register_builtin(*this, "tachyon.transition.circle_iris", "Circle Iris", std::string(circle_iris));
    register_builtin(*this, "tachyon.transition.pixelate", "Pixelate", std::string(pixelate));
    register_builtin(*this, "tachyon.transition.glitch_slice", "Glitch Slice", std::string(glitch_slice));
    register_builtin(*this, "tachyon.transition.rgb_split", "RGB Split", std::string(rgb_split));
    register_builtin(*this, "tachyon.transition.luma_dissolve", "Luma Dissolve", std::string(luma_dissolve));
    register_builtin(*this, "tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", std::string(directional_blur_wipe));
    register_builtin(*this, "tachyon.transition.kaleidoscope", "Kaleidoscope", std::string(kaleidoscope));
    register_builtin(*this, "tachyon.transition.ripple", "Ripple", std::string(ripple));
}

} // namespace tachyon::presets
