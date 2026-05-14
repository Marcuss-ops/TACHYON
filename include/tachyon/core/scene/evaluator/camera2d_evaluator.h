#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/audio/audio_analyzer.h"

namespace tachyon::scene {

/**
 * @brief Evaluates a 2D camera for a given composition and time.
 */
EvaluatedCameraState evaluate_camera2d_state(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    double time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    const EvaluationVariables& vars);

} // namespace tachyon::scene
