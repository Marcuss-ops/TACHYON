#pragma once

#include "tachyon/core/render_graph/render_graph.h"
#include "tachyon/core/profiles/execution_profile.h"
#include <future>

namespace tachyon::ops {

/**
 * @brief High-level media operations for external consumers.
 * Delegating all logic to the Tachyon Core Media system.
 */
class MediaOps {
public:
    /**
     * @brief Executes the full pipeline for a given RenderGraph and profile.
     */
    static core::RenderResult run_pipeline(
        const core::RenderGraph& graph, 
        const core::profiles::ExecutionProfile& profile = core::profiles::ExecutionProfile::preview()
    );

    /**
     * @brief Executes the full pipeline asynchronously.
     */
    static std::future<core::RenderResult> run_pipeline_async(
        const core::RenderGraph& graph,
        const core::profiles::ExecutionProfile& profile = core::profiles::ExecutionProfile::preview()
    );
};

} // namespace tachyon::ops
