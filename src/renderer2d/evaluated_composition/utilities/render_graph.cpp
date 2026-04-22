#include "tachyon/renderer2d/evaluated_composition/render_graph.h"

namespace tachyon::renderer2d {

RenderGraph::RenderGraph(const scene::EvaluatedCompositionState& state) : state_(state) {}

void RenderGraph::buildDependencyGraph() {}

std::vector<DrawCommand2D> RenderGraph::resolveExecutionOrder() {
    return execution_order_;
}

void RenderGraph::topologicalSort() {}

void RenderGraph::detectCycles() {}

} // namespace tachyon::renderer2d
