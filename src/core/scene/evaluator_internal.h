#pragma once

#include "tachyon/core/scene/evaluator.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/text/subtitle.h"
#include "tachyon/media/media_manager.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace tachyon {

namespace scene {

namespace {

struct EvaluationContext {
    const SceneSpec* scene{nullptr};
    const CompositionSpec& composition;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    std::unordered_map<std::string, std::size_t> layer_indices;
    std::vector<std::optional<EvaluatedLayerState>> cache;
    std::vector<bool> visiting;
    std::vector<std::string> composition_stack;
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer{nullptr};
    EvaluationVariables vars;
    std::unordered_map<std::string, std::vector<::tachyon::text::SubtitleEntry>> subtitle_cache;
    ::tachyon::media::MediaManager* media{nullptr};
};

} // namespace

} // namespace scene

} // namespace tachyon