#pragma once

#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/scene/evaluated_state.h"

#include <vector>

namespace tachyon {
namespace renderer2d {

struct RenderContext2D;

class RenderGraph {
public:
    RenderGraph(const scene::EvaluatedCompositionState& state);
    
    void buildDependencyGraph();
    std::vector<DrawCommand2D> resolveExecutionOrder();
    
private:
    const scene::EvaluatedCompositionState& state_;
    std::vector<DrawCommand2D> execution_order_;
    
    void topologicalSort();
    void detectCycles();
};

} // namespace renderer2d
} // namespace tachyon