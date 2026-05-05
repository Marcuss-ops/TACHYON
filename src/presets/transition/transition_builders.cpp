#include "tachyon/presets/transition/transition_builders.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

static registry::ParameterBag to_bag(const TransitionParams& p) {
    registry::ParameterBag bag;
    bag.set("duration", p.duration);
    bag.set("delay", p.delay);
    bag.set("easing", static_cast<int>(p.easing));
    bag.set("direction", p.direction);
    for (const auto& [k, v] : p.parameters) {
        bag.set(k, v);
    }
    return bag;
}

LayerTransitionSpec build_transition_enter(const TransitionParams& p) {
    return TransitionPresetRegistry::instance().create(p.id, to_bag(p));
}

LayerTransitionSpec build_transition_exit(const TransitionParams& p) {
    return TransitionPresetRegistry::instance().create(p.id, to_bag(p));
}

void apply_transitions(LayerSpec& layer,
                       const TransitionParams& enter,
                       const TransitionParams& exit) {
    if (!enter.id.empty()) layer.transition_in  = build_transition_enter(enter);
    if (!exit.id.empty())  layer.transition_out = build_transition_exit(exit);
}

} // namespace tachyon::presets
