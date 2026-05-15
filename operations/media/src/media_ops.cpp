#include "tachyon/media/pipeline_orchestrator.h"
#include "tachyon/core/media/pipeline_orchestrator.h"

namespace tachyon::ops {

core::RenderResult MediaOps::run_pipeline(const core::RenderGraph& graph) {
    // Semplice delega al sistema nervoso centrale (Core)
    return core::PipelineOrchestrator::run(graph);
}

std::future<core::RenderResult> MediaOps::run_pipeline_async(const core::RenderGraph& graph) {
    return core::PipelineOrchestrator::run_async(graph);
}

} // namespace tachyon::ops
