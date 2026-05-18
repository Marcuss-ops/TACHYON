#include "tachyon/runtime/execution/bounds/layer_bounds.h"
#include <algorithm>

namespace tachyon {

LayerBoundsResult LayerBoundsEvaluator::evaluate(
    const CompiledLayer& layer,
    double /*time_seconds*/,
    int composition_width,
    int composition_height
) {
    LayerBoundsResult result;

    const int w = static_cast<int>(layer.width);
    const int h = static_cast<int>(layer.height);

    if (w <= 0 || h <= 0) {
        result.world_bounds = {};
        return result;
    }

    // Base: local bounds of the layer.
    result.local_bounds = {0, 0, w, h};

    // Conservative approach: 
    // until proper transform/matrix evaluation is connected, we assume full frame for complex layers.
    const auto type = layer.kind;

    if (type == LayerType::Precomp ||
        type == LayerType::Video ||
        type == LayerType::Procedural) {
        result.world_bounds = {0, 0, composition_width, composition_height};
        result.full_frame = true;
        return result;
    }

    // Temporary simplification: limit to composition boundaries without transform.
    result.world_bounds = {
        0,
        0,
        std::min(w, composition_width),
        std::min(h, composition_height)
    };

    return result;
}

} // namespace tachyon