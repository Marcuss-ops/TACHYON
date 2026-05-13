#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"

#include <algorithm>
#include <cmath>

namespace tachyon::scene {

namespace {

} // namespace

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {
    return evaluate_composition_internal(
        nullptr,
        composition,
        frame_number,
        composition_time_seconds,
        {},
        audio_analyzer,
        std::move(vars),
        media,
        std::nullopt,
        std::nullopt);
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    double fps = composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;
    double time = static_cast<double>(frame_number) / fps;
    return evaluate_composition_state(composition, frame_number, time, audio_analyzer, vars, media);
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    double fps = composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;
    std::int64_t frame = static_cast<std::int64_t>(composition_time_seconds * fps);
    return evaluate_composition_state(composition, frame, composition_time_seconds, audio_analyzer, vars, media);
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    for (const auto& comp : scene.compositions) {
        if (comp.id == composition_id) {
            double fps = comp.frame_rate.value();
            if (fps <= 0.0) fps = 24.0;
            double time = static_cast<double>(frame_number) / fps;
            return evaluate_composition_internal(
                &scene, comp, frame_number, time,
                {}, audio_analyzer, vars, media, std::nullopt, std::nullopt);
        }
    }
    return std::nullopt;
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    for (const auto& comp : scene.compositions) {
        if (comp.id == composition_id) {
            double fps = comp.frame_rate.value();
            if (fps <= 0.0) fps = 24.0;
            std::int64_t frame = static_cast<std::int64_t>(composition_time_seconds * fps);
            return evaluate_composition_internal(
                &scene, comp, frame, composition_time_seconds,
                {}, audio_analyzer, vars, media, std::nullopt, std::nullopt);
        }
    }
    return std::nullopt;
}

EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    media::MediaManager* media) {
    CompositionSpec composition;
    composition.frame_rate = {30, 1};
    composition.layers.push_back(layer);

    EvaluationContext context{
        .scene = nullptr,
        .composition = composition,
        .frame_number = frame_number,
        .composition_time_seconds = composition_time_seconds,
        .layer_indices = {},
        .camera2d_indices = {},
        .cache = std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        .visiting = std::vector<bool>(composition.layers.size(), false),
        .composition_stack = {},
        .audio_analyzer = audio_analyzer,
        .vars = {},
        .subtitle_cache = {},
        .media = media,
        .sampler = nullptr,
        .main_frame_number = std::nullopt,
        .main_frame_time_seconds = std::nullopt
    };

    return make_layer_state(context, context.composition.layers.front(), 0, 0.0, {});
}

} // namespace tachyon::scene
