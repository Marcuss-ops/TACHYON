#include "tachyon/core/scene/evaluator/precomp_evaluator.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"
#include <cmath>

namespace tachyon::scene {

void evaluate_precomp_layer(
    const SceneSpec* scene,
    const EvaluatedLayerState& base_layer,
    EvaluationContext& context,
    EvaluatedLayerState& output_layer) {
    
    if (!output_layer.source.precomp_id.has_value() || output_layer.source.precomp_id->empty() || !scene) {
        return;
    }

    bool circular = false;
    for (const auto& id : context.composition_stack) {
        if (id == *output_layer.source.precomp_id) {
            circular = true;
            break;
        }
    }

    if (!circular) {
        for (const auto& comp : scene->compositions) {
            if (comp.id == *output_layer.source.precomp_id) {
                std::vector<std::string> next_stack = context.composition_stack;
                next_stack.push_back(context.composition.id);
                
                const std::int64_t child_frame_number = static_cast<std::int64_t>(std::llround(
                    output_layer.playback.local_time_seconds * 
                    static_cast<double>(comp.frame_rate.numerator) / 
                    static_cast<double>(comp.frame_rate.denominator)
                ));

                output_layer.nested_composition = std::make_unique<EvaluatedCompositionState>(
                    evaluate_composition_internal(
                        scene, 
                        comp, 
                        child_frame_number, 
                        output_layer.playback.local_time_seconds, 
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
