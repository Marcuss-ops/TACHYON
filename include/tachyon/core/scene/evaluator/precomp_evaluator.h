#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/scene/evaluation/evaluator.h"

namespace tachyon::scene {

/**
 * @brief Evaluates a nested precomposition for a layer.
 */
void evaluate_precomp_layer(
    const SceneSpec* scene,
    const EvaluatedLayerState& base_layer,
    EvaluationContext& context,
    EvaluatedLayerState& output_layer);

} // namespace tachyon::scene
