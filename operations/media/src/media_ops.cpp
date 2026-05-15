#include "tachyon/ops/media_ops.h"
#include "tachyon/runtime/media/media_pipeline.h"

namespace tachyon::ops {

core::RenderResult MediaOps::run_pipeline(const core::RenderGraph& graph) {
    // Operations delegate to the Runtime Orchestrator
    return runtime::media::MediaPipeline::run(graph);
}

std::future<core::RenderResult> MediaOps::run_pipeline_async(const core::RenderGraph& graph) {
    return runtime::media::MediaPipeline::run_async(graph);
}

} // namespace tachyon::ops
