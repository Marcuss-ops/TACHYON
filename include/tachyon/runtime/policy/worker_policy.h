#pragma once

#include "tachyon/runtime/policy/worker_budget.h"
#include <cstddef>

namespace tachyon::runtime {

/**
 * @brief Policy for resolving the number of worker threads to use for rendering.
 * 
 * This ensures that hardware concurrency is not used directly in the runtime logic,
 * allowing for overrides, constraints, and deterministic defaults.
 */
struct RenderWorkerPolicy {
    std::size_t requested_workers{0};  // 0 means use hardware defaults
    std::size_t max_workers{0};        // 0 means no limit
    std::size_t min_workers{1};        // absolute minimum

    /**
     * @brief Resolves the final worker budget based on hardware capabilities and policy constraints.
     */
    RenderWorkerBudget resolve(std::size_t hardware_threads) const;
};

} // namespace tachyon::runtime
