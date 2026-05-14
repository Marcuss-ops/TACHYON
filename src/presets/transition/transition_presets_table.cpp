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
        return TransitionPresetRegistry::build_spec_from_params(p, canonical_id);
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

    register_builtin(*this, "tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", std::string(directional_blur_wipe));

    register_builtin(*this, "tachyon.transition.ripple", "Ripple", std::string(ripple));

    // Modern V2
    register_builtin(*this, "tachyon.transition.smooth_wipe", "Smooth Wipe", std::string(smooth_wipe), {"wipe", "modern", "smooth"});
    register_builtin(*this, "tachyon.transition.soft_zoom_blur", "Soft Zoom Blur", std::string(soft_zoom_blur), {"zoom", "modern", "blur"});
    register_builtin(*this, "tachyon.transition.flash_cut", "Flash Cut", std::string(flash_cut), {"flash", "cinematic", "fast"});

    // Premium Light Leaks
    register_builtin(*this, "tachyon.transition.lightleak.soft_warm_edge", "Soft Warm Edge", std::string(lightleak_soft_warm_edge), {"lightleak", "premium", "vfx"});
    register_builtin(*this, "tachyon.transition.lightleak.golden_sweep", "Golden Sweep", std::string(lightleak_golden_sweep), {"lightleak", "premium", "vfx"});
    register_builtin(*this, "tachyon.transition.lightleak.creamy_white", "Creamy White", std::string(lightleak_creamy_white), {"lightleak", "premium", "vfx"});
    register_builtin(*this, "tachyon.transition.lightleak.dusty_archive", "Dusty Archive", std::string(lightleak_dusty_archive), {"lightleak", "premium", "vfx"});
    register_builtin(*this, "tachyon.transition.lightleak.lens_flare_pass", "Lens Flare Pass", std::string(lightleak_lens_flare_pass), {"lightleak", "premium", "vfx"});
}

} // namespace tachyon::presets
