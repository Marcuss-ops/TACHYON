#pragma once

#include "tachyon/core/scene/evaluator.h"

namespace tachyon::scene {

EvaluatedLightState evaluate_light_state(
    const EvaluatedLayerState& layer_state,
    const LayerSpec& spec,
    double remapped_time);

} // namespace tachyon::scene
