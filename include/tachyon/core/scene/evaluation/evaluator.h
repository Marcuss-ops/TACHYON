#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/runtime/execution/jobs/render_job.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace tachyon::media {
class MediaManager;
}

namespace tachyon::audio {
class AudioAnalyzer;
}

#include <functional>

namespace tachyon::scene {

typedef std::function<double(int, double)> PropertySampler;

struct EvaluationVariables {
    const std::unordered_map<std::string, double>* numeric{nullptr};
    const std::unordered_map<std::string, std::string>* strings{nullptr};
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables{nullptr};
    const std::unordered_map<std::string, RenderJob::LayerOverride>* layer_overrides{nullptr};
};

[[nodiscard]] TACHYON_API EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer = nullptr,
    media::MediaManager* media = nullptr
);

[[nodiscard]] TACHYON_API EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds
);

[[nodiscard]] TACHYON_API EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::MediaManager* media = nullptr
);

[[nodiscard]] TACHYON_API EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::MediaManager* media = nullptr
);

[[nodiscard]] TACHYON_API std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::MediaManager* media = nullptr
);

[[nodiscard]] TACHYON_API std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::MediaManager* media = nullptr
);

} // namespace tachyon::scene
