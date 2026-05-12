#pragma once

#include "tachyon/core/timeline/timeline_plan.h"
#include "tachyon/core/render_graph/render_result.h"
#include <string>
#include <memory>
#include <utility>

namespace tachyon::core {

/**
 * @brief Operational mode for the render pipeline.
 */
enum class RenderMode {
    Normal,
    Fast,
    ValidationOnly
};

/**
 * @brief Configuration for pipeline execution behavior.
 */
struct RenderConfig {
    RenderMode mode{RenderMode::Normal};
    double timeout_seconds{300.0};
    bool emit_drift_metrics{true};
    bool emit_component_metrics{true};
};

/**
 * @brief Unified container for a render job and its execution state.
 * Derived from ruststream-core/core/render_graph/graph.rs
 */
class RenderGraph {
public:
    std::string graph_id;
    std::shared_ptr<MediaTimelinePlan> timeline;
    RenderConfig config;
    bool emergency_mode{false};

    explicit RenderGraph(std::string id, std::shared_ptr<MediaTimelinePlan> plan)
        : graph_id(std::move(id)), timeline(std::move(plan)) {}

    /**
     * @brief Performs global validation of the graph structure.
     */
    [[nodiscard]] bool validate() const {
        return !graph_id.empty() && timeline != nullptr && timeline->validate();
    }
};

} // namespace tachyon::core
