#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/audio/audio_interfaces.h"
#include "tachyon/core/scene/internal/evaluator_internal.h"

namespace tachyon::scene {

/**
 * @brief Evaluates and expands repeaters for a given layer.
 */
void evaluate_repeater_expansion(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    std::size_t layer_index,
    const EvaluatedLayerState& base_layer,
    EvaluationContext& context,
    std::vector<EvaluatedLayerState>& output_layers);

} // namespace tachyon::scene
