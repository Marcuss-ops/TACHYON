#pragma once

#include "tachyon/spec/scene_spec.h"
#include "tachyon/timeline/evaluated_state.h"

#include <cstdint>
#include <optional>
#include <string>

namespace tachyon::timeline {

struct EvaluationRequest {
    const SceneSpec* scene{nullptr};
    std::string composition_id;
    std::int64_t frame_number{0};
};

std::optional<EvaluatedCompositionState> evaluate_composition_frame(const EvaluationRequest& request);

double frame_to_seconds(std::int64_t frame_number, const FrameRate& frame_rate);
bool is_layer_visible_at_time(const LayerSpec& layer, double composition_time_seconds, double composition_duration_seconds);

} // namespace tachyon::timeline
