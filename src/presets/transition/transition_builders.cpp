#include "tachyon/presets/transition/transition_builders.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

LayerTransitionSpec build_transition_enter(const TransitionParams& p) {
    return TransitionPresetRegistry::instance().create(p.id, p);
}

LayerTransitionSpec build_transition_exit(const TransitionParams& p) {
    return TransitionPresetRegistry::instance().create(p.id, p);
}

void apply_transitions(LayerSpec& layer,
                       const TransitionParams& enter,
                       const TransitionParams& exit) {
    if (!enter.id.empty()) layer.transition_in  = build_transition_enter(enter);
    if (!exit.id.empty())  layer.transition_out = build_transition_exit(exit);
}

} // namespace tachyon::presets
