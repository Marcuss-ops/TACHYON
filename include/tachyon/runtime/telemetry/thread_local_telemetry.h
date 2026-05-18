#pragma once
#include <cstdint>
#include <memory>
#include <atomic>

namespace tachyon {
namespace runtime {

/**
 * @brief Thread-local accumulator for high-frequency render execution metrics.
 */
struct ThreadLocalTelemetry {
    std::uint64_t pixels_processed{0};
    std::uint64_t tiles_processed{0};

    void reset() noexcept {
        pixels_processed = 0;
        tiles_processed = 0;
    }
};

// Thread-local global instance
extern thread_local ThreadLocalTelemetry tl_telemetry;

/**
 * @brief Safely flushes the accumulated thread-local metrics into shared atomic counters.
 */
inline void flush_thread_local_telemetry(
    std::shared_ptr<std::atomic<std::size_t>> total_pixels_counter,
    std::shared_ptr<std::atomic<int>> total_tiles_counter) noexcept {
    if (tl_telemetry.pixels_processed > 0 && total_pixels_counter) {
        total_pixels_counter->fetch_add(tl_telemetry.pixels_processed, std::memory_order_relaxed);
    }
    if (tl_telemetry.tiles_processed > 0 && total_tiles_counter) {
        total_tiles_counter->fetch_add(static_cast<int>(tl_telemetry.tiles_processed), std::memory_order_relaxed);
    }
    tl_telemetry.reset();
}

} // namespace runtime
} // namespace tachyon
