#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace tachyon::core {

/**
 * @brief Metrics for an individual pipeline component.
 */
struct ComponentMetrics {
    int attempts{0};
    int successes{0};
    int fallbacks{0};
};

/**
 * @brief Unified result for a complete render graph execution.
 * Derived from ruststream-core/core/render_graph/result.rs
 */
struct RenderResult {
    bool success{false};
    std::string artifact_path;
    
    // Telemetry and Metrics
    std::map<std::string, ComponentMetrics> metrics;
    double drift{0.0};
    std::vector<std::string> reason_codes;
    
    // Error Context (if success is false)
    std::string error_message;
    std::string error_code;
    std::string error_stage;
    
    // Fallback Information
    bool fallback_used{false};
    std::vector<std::string> fallback_components;

    /**
     * @brief Returns true if the render succeeded without using any fallbacks.
     */
    [[nodiscard]] bool is_clean_success() const {
        return success && !fallback_used;
    }
    
    static RenderResult failure(const MediaError& err) {
        RenderResult res;
        res.success = false;
        res.error_message = err.message;
        res.error_code = std::string(to_string(err.code));
        if (err.stage) res.error_stage = *err.stage;
        return res;
    }
};

} // namespace tachyon::core
