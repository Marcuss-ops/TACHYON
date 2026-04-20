#pragma once

#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/core/scene/evaluated_state.h"
#include "tachyon/timeline/time.h"

#include <cstdint>
#include <optional>
#include <string>

namespace tachyon::timeline {

struct EvaluationRequest {
    const SceneSpec* scene{nullptr};
    std::string composition_id;
    std::int64_t frame_number{0};
};

std::optional<scene::EvaluatedCompositionState> evaluate_composition_frame(const EvaluationRequest& request);

bool is_layer_visible_at_time(const LayerSpec& layer, double composition_time_seconds, double composition_duration_seconds);

} // namespace tachyon::timeline
