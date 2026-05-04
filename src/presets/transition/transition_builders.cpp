#include "tachyon/presets/transition/transition_builders.h"

namespace tachyon::presets {

namespace {

LayerTransitionSpec make_spec(const TransitionParams& p) {
    LayerTransitionSpec spec;
    spec.transition_id = p.id;
    spec.type          = p.id.empty() ? "none" : p.id;
    spec.duration      = p.duration;
    spec.easing        = p.easing;
    spec.delay         = p.delay;
    spec.direction     = p.direction;
    return spec;
}

} // namespace

LayerTransitionSpec build_transition_enter(const TransitionParams& p) {
    return make_spec(p);
}

LayerTransitionSpec build_transition_exit(const TransitionParams& p) {
    return make_spec(p);
}

void apply_transitions(LayerSpec& layer,
                       const TransitionParams& enter,
                       const TransitionParams& exit) {
    if (!enter.id.empty()) layer.transition_in  = build_transition_enter(enter);
    if (!exit.id.empty())  layer.transition_out = build_transition_exit(exit);
}

} // namespace tachyon::presets
