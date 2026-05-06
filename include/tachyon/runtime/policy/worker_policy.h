#pragma once

#include <cstddef>

namespace tachyon::runtime {

/**
 * @brief Policy for resolving the number of worker threads to use for rendering.
 * 
 * This ensures that hardware concurrency is not used directly in the runtime logic,
 * allowing for overrides, constraints, and deterministic defaults.
 */
struct WorkerPolicy {
    std::size_t requested_workers{0};  // 0 means use hardware defaults
    std::size_t max_workers{0};        // 0 means no limit
    std::size_t min_workers{1};        // absolute minimum

    /**
     * @brief Resolves the final worker count based on hardware capabilities and policy constraints.
     */
    std::size_t resolve(std::size_t hardware_threads) const;
};

} // namespace tachyon::runtime
