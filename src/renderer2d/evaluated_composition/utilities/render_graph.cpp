#include "tachyon/renderer2d/evaluated_composition/renderer2d_render_graph.h"

namespace tachyon::renderer2d {

Renderer2DRenderGraph::Renderer2DRenderGraph(const scene::EvaluatedCompositionState& state) : state_(state) {}

void Renderer2DRenderGraph::buildDependencyGraph() {}

std::vector<DrawCommand2D> Renderer2DRenderGraph::resolveExecutionOrder() {
    return execution_order_;
}

void Renderer2DRenderGraph::topologicalSort() {}

void Renderer2DRenderGraph::detectCycles() {}

} // namespace tachyon::renderer2d
