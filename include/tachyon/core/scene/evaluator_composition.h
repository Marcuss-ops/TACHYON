#pragma once

#include "tachyon/core/scene/evaluator_internal.h"
#include "tachyon/core/scene/evaluator_layer.h"
#include "tachyon/core/scene/evaluator.h"

namespace tachyon {
namespace scene {

const EvaluatedLayerState& resolve_layer_state(
    std::size_t layer_index,
    EvaluationContext& context);

EvaluatedCompositionState evaluate_composition_internal(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    std::int64_t frame_number,
    double composition_time_seconds,
    std::vector<std::string> stack,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media,
    std::optional<std::int64_t> main_frame_number = std::nullopt,
    std::optional<double> main_frame_time_seconds = std::nullopt);

} // namespace scene
} // namespace tachyon
