#include "tachyon/core/scene/evaluator/layer_dependency_resolver.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"

namespace tachyon::scene {

void resolve_layer_dependencies(
    std::size_t layer_index,
    EvaluationContext& context,
    EvaluatedLayerState& evaluated) {
    
    const auto& layer_spec = context.composition.layers[layer_index];

    // 1. Parenting
    if (layer_spec.parent.has_value() && !layer_spec.parent->empty()) {
        const auto parent_it = context.layer_indices.find(*layer_spec.parent);
        if (parent_it != context.layer_indices.end() && parent_it->second != layer_index) {
            const auto& parent = resolve_layer_state(parent_it->second, context);
            evaluated.transform.world_matrix = parent.transform.world_matrix * evaluated.transform.world_matrix;
            evaluated.identity.visible = evaluated.identity.enabled && evaluated.identity.active && parent.identity.visible && evaluated.transform.opacity > 0.0;
        }
    }

    // 2. Track Matte
    if (layer_spec.track_matte_layer_id.has_value() && !layer_spec.track_matte_layer_id->empty()) {
        const auto matte_it = context.layer_indices.find(*layer_spec.track_matte_layer_id);
        if (matte_it != context.layer_indices.end()) {
            evaluated.track_matte_layer_index = matte_it->second;
        }
    }
}

} // namespace tachyon::scene
