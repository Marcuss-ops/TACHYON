#pragma once

#include "tachyon/core/scene/evaluator.h"
#include "tachyon/core/scene/evaluator_internal.h"

namespace tachyon::scene {

EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& layer,
    std::size_t layer_index,
    double time_offset = 0.0);

} // namespace tachyon::scene
