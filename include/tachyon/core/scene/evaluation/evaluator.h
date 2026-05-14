#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <functional>

namespace tachyon::media {
class IMediaProvider;
}

namespace tachyon::audio {
class IAudioAnalyzer;
}

namespace tachyon::scene {

typedef std::function<double(int, double)> PropertySampler;

struct EvaluationVariables {
    const std::unordered_map<std::string, double>* numeric{nullptr};
    const std::unordered_map<std::string, std::string>* strings{nullptr};
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables{nullptr};
    const std::unordered_map<std::string, RenderJob::LayerOverride>* layer_overrides{nullptr};
};

[[nodiscard]] EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::IAudioAnalyzer* audio_analyzer = nullptr,
    media::IMediaProvider* media = nullptr
);


[[nodiscard]] EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const audio::IAudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::IMediaProvider* media = nullptr
);

[[nodiscard]] EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const audio::IAudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::IMediaProvider* media = nullptr
);

[[nodiscard]] std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const audio::IAudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::IMediaProvider* media = nullptr
);

[[nodiscard]] std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const audio::IAudioAnalyzer* audio_analyzer = nullptr,
    EvaluationVariables vars = {},
    media::IMediaProvider* media = nullptr
);

} // namespace tachyon::scene
