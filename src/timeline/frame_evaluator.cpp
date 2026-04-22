#include "tachyon/timeline/evaluator.h"
#include "tachyon/core/scene/evaluation/evaluator.h"

namespace tachyon::timeline {

std::optional<scene::EvaluatedCompositionState> evaluate_composition_frame(const EvaluationRequest& request) {
    if (!request.scene) {
        return std::nullopt;
    }
    return scene::evaluate_scene_composition_state(*request.scene, request.composition_id, request.frame_number);
}

} // namespace tachyon::timeline
