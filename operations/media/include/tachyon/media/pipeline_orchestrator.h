#pragma once

#include "tachyon/core/render_graph/render_graph.h"
#include <future>

namespace tachyon::ops {

/**
 * @brief High-level media pipeline operations for external consumers.
 * Wraps the core PipelineOrchestrator nervous system.
 */
class MediaOps {
public:
    /**
     * @brief Run a full media pipeline job.
     */
    static core::RenderResult run_pipeline(const core::RenderGraph& graph);

    /**
     * @brief Run a full media pipeline job asynchronously.
     */
    static std::future<core::RenderResult> run_pipeline_async(const core::RenderGraph& graph);
};

} // namespace tachyon::ops
