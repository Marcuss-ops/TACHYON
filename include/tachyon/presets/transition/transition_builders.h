#pragma once

#include "tachyon/presets/transition/transition_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

namespace tachyon::presets {

// Restituisce un LayerTransitionSpec da applicare come enter o exit di un layer.
// Uso: layer.transition_in  = build_transition_enter({.id="slide_up", .duration=0.3});
//      layer.transition_out = build_transition_exit ({.id="fade",     .duration=0.2});

[[nodiscard]] LayerTransitionSpec build_transition_enter(const TransitionParams& p);
[[nodiscard]] LayerTransitionSpec build_transition_exit(const TransitionParams& p);

// Applica enter e/o exit direttamente su un LayerSpec esistente.
void apply_transitions(LayerSpec& layer,
                       const TransitionParams& enter,
                       const TransitionParams& exit = {});

} // namespace tachyon::presets
