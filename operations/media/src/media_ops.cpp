#include "tachyon/ops/media_ops.h"
#include "tachyon/runtime/media/media_pipeline.h"
#include "tachyon/backends/backend_registry.h"
#include <memory>

namespace tachyon::ops {

core::RenderResult MediaOps::run_pipeline(const core::RenderGraph& graph, const core::profiles::ExecutionProfile& profile) {
    auto& reg = backends::BackendRegistry::instance();

    // Resolve services based on profile
    auto probe = reg.create_probe(profile.metadata.count("probe") ? profile.metadata.at("probe") : "default");
    auto clip_processor = reg.create_clip_processor();
    auto overlay_merger = reg.create_overlay_merger();
    auto audio_extractor = reg.create_audio_extractor();
    auto audio_analyzer = reg.create_audio_analyzer(profile.audio_analyzer);
    auto video_concat = reg.create_video_concat();
    auto transition_renderer = reg.create_transition_renderer();

    // Ensure all required services are available
    if (!probe || !clip_processor || !overlay_merger || !audio_extractor || !audio_analyzer || !video_concat || !transition_renderer) {
        core::RenderResult failure;
        failure.success = false;
        failure.error_message = "Failed to resolve required media services from BackendRegistry";
        return failure;
    }

    // Bundle services for the runtime pipeline
    runtime::media::MediaServices services {
        *probe,
        *clip_processor,
        *overlay_merger,
        *audio_extractor,
        *audio_analyzer,
        *video_concat,
        *transition_renderer
    };
    
    // Delegate to the Runtime Orchestrator
    return runtime::media::MediaPipeline::run(graph, services);
}

std::future<core::RenderResult> MediaOps::run_pipeline_async(const core::RenderGraph& graph, const core::profiles::ExecutionProfile& profile) {
    return std::async(std::launch::async, [graph, profile]() {
        return run_pipeline(graph, profile);
    });
}

} // namespace tachyon::ops
