#pragma once

#include "tachyon/renderer2d/geometry/dirty_region.h"
#include "tachyon/runtime/core/data/compiled_scene.h"

namespace tachyon {

struct LayerBoundsResult {
    renderer2d::IntRect local_bounds;
    renderer2d::IntRect world_bounds;
    bool full_frame{false};
};

class LayerBoundsEvaluator {
public:
    static LayerBoundsResult evaluate(
        const CompiledLayer& layer,
        double time_seconds,
        int composition_width,
        int composition_height
    );
};

} // namespace tachyon