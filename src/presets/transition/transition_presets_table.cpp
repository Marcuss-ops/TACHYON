#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"

namespace tachyon::presets {

namespace {
 
void register_builtin(
    tachyon::TransitionRegistry& registry,
    const std::string& id,
    const std::string& name,
    TransitionKind kind,
    TransitionRuntimeKind runtime_kind,
    const std::vector<std::string>& tags = {"cinematic"}) 
{
    TransitionDescriptor desc;
    desc.id = id;
    desc.display_name = name;
    desc.description = name + " transition effect";
    desc.category = static_cast<TransitionCategory>(kind);
    desc.runtime_kind = runtime_kind;
    desc.capabilities.supports_cpu = (runtime_kind == TransitionRuntimeKind::CpuPixel || runtime_kind == TransitionRuntimeKind::StateOnly);
    desc.capabilities.supports_gpu = (runtime_kind == TransitionRuntimeKind::Glsl);
    registry.register_descriptor(desc);
}
 
} // namespace

void TransitionPresetRegistry::load_builtins(tachyon::TransitionRegistry& registry) {
    // Utility
    register_builtin(registry, "tachyon.transition.none", "None", TransitionKind::None, TransitionRuntimeKind::StateOnly, {"utility"});

    // Canonical presets backed by verified library assets
    register_builtin(registry, "tachyon.transition.crossfade", "Crossfade", TransitionKind::Fade, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.slide_up", "Slide Up", TransitionKind::Slide, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.swipe_left", "Swipe Left", TransitionKind::Wipe, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.fade_to_black", "Fade to Black", TransitionKind::Fade, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.wipe_linear", "Wipe Linear", TransitionKind::Wipe, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.wipe_angular", "Wipe Angular", TransitionKind::Wipe, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.push_left", "Push Left", TransitionKind::Slide, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.slide_easing", "Slide Easing", TransitionKind::Slide, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.zoom_in", "Zoom In", TransitionKind::Zoom, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.zoom_blur", "Zoom Blur", TransitionKind::Zoom, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.spin", "Spin", TransitionKind::Flip, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.circle_iris", "Circle Iris", TransitionKind::Wipe, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.pixelate", "Pixelate", TransitionKind::Custom, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.glitch_slice", "Glitch Slice", TransitionKind::Custom, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.rgb_split", "RGB Split", TransitionKind::Custom, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.luma_dissolve", "Luma Dissolve", TransitionKind::Dissolve, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.directional_blur_wipe", "Directional Blur Wipe", TransitionKind::Wipe, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.kaleidoscope", "Kaleidoscope", TransitionKind::Custom, TransitionRuntimeKind::Glsl);
    register_builtin(registry, "tachyon.transition.ripple", "Ripple", TransitionKind::Custom, TransitionRuntimeKind::Glsl);
}

} // namespace tachyon::presets
