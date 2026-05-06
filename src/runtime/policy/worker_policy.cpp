#include "tachyon/runtime/policy/worker_policy.h"

#include <algorithm>

namespace tachyon::runtime {

std::size_t WorkerPolicy::resolve(std::size_t hardware_threads) const {
    const std::size_t safe_hardware = (hardware_threads == 0) ? 1 : hardware_threads;

    std::size_t workers = (requested_workers > 0)
        ? requested_workers
        : safe_hardware;

    if (max_workers > 0) {
        workers = std::min(workers, max_workers);
    }

    workers = std::max(workers, min_workers);
    return workers;
}

} // namespace tachyon::runtime
