#pragma once

#include "tachyon/scene/evaluated_state.h"

#include <optional>
#include <string>

namespace tachyon::scene {

[[nodiscard]] EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds
);

[[nodiscard]] EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds
);

[[nodiscard]] EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number
);

[[nodiscard]] std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number
);

} // namespace tachyon::scene
