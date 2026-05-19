#pragma once

#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/audio/audio_interfaces.h"
#include "tachyon/core/media/media_provider.h"
#ifdef TACHYON_ENABLE_TEXT
#include "tachyon/text/content/subtitle.h"
#endif

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
    const ::tachyon::audio::IAudioAnalyzer* audio_analyzer{nullptr};
    EvaluationVariables vars;
#ifdef TACHYON_ENABLE_TEXT
    std::unordered_map<std::string, std::vector<::tachyon::text::SubtitleEntry>> subtitle_cache;
#endif
    ::tachyon::media::IMediaProvider* media{nullptr};
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
