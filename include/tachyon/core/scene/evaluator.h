#pragma once

#include "tachyon/core/scene/evaluated_state.h"

#include <optional>
#include <string>

namespace tachyon::audio {
class AudioAnalyzer;
}

namespace tachyon::scene {

[[nodiscard]] EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer = nullptr
);

[[nodiscard]] EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds
);

[[nodiscard]] EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer = nullptr
);

[[nodiscard]] EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer = nullptr
);

[[nodiscard]] std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer = nullptr
);

[[nodiscard]] std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer = nullptr
);

} // namespace tachyon::scene
