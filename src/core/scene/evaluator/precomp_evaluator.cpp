#include "tachyon/core/scene/evaluator/precomp_evaluator.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"
#include <cmath>

namespace tachyon::scene {

void attach_nested_precomp(
    EvaluationContext& context,
    EvaluatedLayerState& layer_state) {
    
    const auto* precomp = std::get_if<PrecompSource>(&context.composition.layers[context.current_layer_index].source);
    if (!precomp || precomp->precomp_id.empty() || !context.scene) {
        return;
    }

    bool circular = false;
    for (const auto& id : context.composition_stack) {
        if (id == precomp->precomp_id) {
            circular = true;
            break;
        }
    }

    if (!circular) {
        for (const auto& comp : context.scene->compositions) {
            if (comp.id == precomp->precomp_id) {
                std::vector<std::string> next_stack = context.composition_stack;
                next_stack.push_back(context.composition.id);
                
                const std::int64_t child_frame_number = static_cast<std::int64_t>(std::llround(
                    layer_state.playback.local_time_seconds * 
                    static_cast<double>(comp.frame_rate.numerator) / 
                    static_cast<double>(comp.frame_rate.denominator)
                ));

                layer_state.nested_composition = std::make_unique<EvaluatedCompositionState>(
                    evaluate_composition_internal(
                        context.scene, 
                        comp, 
                        child_frame_number, 
                        layer_state.playback.local_time_seconds, 
                        std::move(next_stack), 
                        context.audio_analyzer, 
                        context.vars, 
                        context.media, 
                        context.main_frame_number, 
                        context.main_frame_time_seconds
                    )
                );
                break;
            }
        }
    }
}

} // namespace tachyon::scene
