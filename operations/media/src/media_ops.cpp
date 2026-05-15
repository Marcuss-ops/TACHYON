#include "tachyon/ops/media_ops.h"
#include "tachyon/runtime/media/media_pipeline.h"
#include "tachyon/backends/media_backend_bundle.h"

namespace tachyon::ops {

core::RenderResult MediaOps::run_pipeline(const core::RenderGraph& graph) {
    // Operations provide the concrete implementation bundle
    backends::MediaBackendBundle bundle;
    auto services = bundle.services();
    
    // Delegate to the Runtime Orchestrator using injected services
    return runtime::media::MediaPipeline::run(graph, services);
}

std::future<core::RenderResult> MediaOps::run_pipeline_async(const core::RenderGraph& graph) {
    // Note: bundle must live as long as the async task if we use references.
    // In a real studio engine, services would be managed by a ServiceLocator or lifetime-managed container.
    // For now, we instantiate inside the future or use a shared bundle.
    return std::async(std::launch::async, [graph]() {
        backends::MediaBackendBundle bundle;
        auto services = bundle.services();
        return runtime::media::MediaPipeline::run(graph, services);
    });
}

} // namespace tachyon::ops
