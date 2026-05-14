#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/scene/evaluation/evaluator.h"

namespace tachyon::scene {

/**
 * @brief Resolves layer dependencies such as parenting and track mattes.
 */
void resolve_layer_dependencies(
    std::size_t layer_index,
    EvaluationContext& context,
    EvaluatedLayerState& evaluated);

} // namespace tachyon::scene
