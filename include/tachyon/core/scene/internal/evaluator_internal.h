#pragma once

#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/text/content/subtitle.h"
#include "tachyon/media/management/media_manager.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

namespace tachyon {
namespace scene {

struct EvaluationContext {
    const SceneSpec* scene{nullptr};
    const CompositionSpec& composition;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    std::unordered_map<std::string, std::size_t> layer_indices;
    std::unordered_map<std::string, std::size_t> camera2d_indices;
    std::vector<std::optional<EvaluatedLayerState>> cache;
    std::vector<bool> visiting;
    std::size_t current_layer_index{0};
    std::vector<std::string> composition_stack;
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer{nullptr};
    EvaluationVariables vars;
    std::unordered_map<std::string, std::vector<::tachyon::text::SubtitleEntry>> subtitle_cache;
    ::tachyon::media::MediaManager* media{nullptr};
    PropertySampler sampler{nullptr};
    
    // Temporal context for selective motion blur
    std::optional<std::int64_t> main_frame_number;
    std::optional<double> main_frame_time_seconds;
};

// Internal evaluation primitives
const EvaluatedLayerState& resolve_layer_state(
    std::size_t layer_index,
    EvaluationContext& context);

void attach_nested_precomp(
    EvaluationContext& context,
    EvaluatedLayerState& layer_state);

void resolve_layer_dependencies(
    std::size_t layer_index,
    EvaluationContext& context,
    EvaluatedLayerState& evaluated);

} // namespace scene
} // namespace tachyon
