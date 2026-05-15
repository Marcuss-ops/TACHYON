#pragma once

#include "tachyon/core/render_graph/render_graph.h"
#include <future>

namespace tachyon::ops {

/**
 * @brief High-level media operations for external consumers.
 * Delegating all logic to the Tachyon Core Media system.
 */
class MediaOps {
public:
    /**
     * @brief Executes the full pipeline for a given RenderGraph.
     */
    static core::RenderResult run_pipeline(const core::RenderGraph& graph);

    /**
     * @brief Executes the full pipeline asynchronously.
     */
    static std::future<core::RenderResult> run_pipeline_async(const core::RenderGraph& graph);
};

} // namespace tachyon::ops
