#pragma once
#include <cstdint>
#include <memory>
#include <atomic>

namespace tachyon {
namespace runtime {

/**
 * @brief Thread-local accumulator for high-frequency render execution metrics.
 * 
 * Note: pixel_ops measures cumulative pixel operations performed by the render 
 * engine (including intermediate layers, solid/image fills, and blending operations).
 * rasterized_pixels tracks unique pixels drawn from scene geometry.
 * blend_pixel_ops tracks pixels processed during frame blending or layer compositing.
 */
struct ThreadLocalTelemetry {
    std::uint64_t pixel_ops{0};
    std::uint64_t rasterized_pixels{0};
    std::uint64_t blend_pixel_ops{0};
    std::uint64_t encoded_pixels{0};
    std::uint64_t tiles_processed{0};

    void reset() noexcept {
        pixel_ops = 0;
        rasterized_pixels = 0;
        blend_pixel_ops = 0;
        encoded_pixels = 0;
        tiles_processed = 0;
    }
};

// Thread-local global instance
extern thread_local ThreadLocalTelemetry tl_telemetry;

/**
 * @brief Safely flushes the accumulated thread-local metrics into shared atomic counters.
 */
inline void flush_thread_local_telemetry(
    std::shared_ptr<std::atomic<std::size_t>> total_pixel_ops_counter,
    std::shared_ptr<std::atomic<int>> total_tiles_counter,
    std::shared_ptr<std::atomic<std::size_t>> rasterized_pixels_counter = nullptr,
    std::shared_ptr<std::atomic<std::size_t>> blend_pixels_counter = nullptr,
    std::shared_ptr<std::atomic<std::size_t>> encoded_pixels_counter = nullptr) noexcept {
    
    if (tl_telemetry.pixel_ops > 0 && total_pixel_ops_counter) {
        total_pixel_ops_counter->fetch_add(tl_telemetry.pixel_ops, std::memory_order_relaxed);
    }
    if (tl_telemetry.rasterized_pixels > 0 && rasterized_pixels_counter) {
        rasterized_pixels_counter->fetch_add(tl_telemetry.rasterized_pixels, std::memory_order_relaxed);
    }
    if (tl_telemetry.blend_pixel_ops > 0 && blend_pixels_counter) {
        blend_pixels_counter->fetch_add(tl_telemetry.blend_pixel_ops, std::memory_order_relaxed);
    }
    if (tl_telemetry.encoded_pixels > 0 && encoded_pixels_counter) {
        encoded_pixels_counter->fetch_add(tl_telemetry.encoded_pixels, std::memory_order_relaxed);
    }
    if (tl_telemetry.tiles_processed > 0 && total_tiles_counter) {
        total_tiles_counter->fetch_add(static_cast<int>(tl_telemetry.tiles_processed), std::memory_order_relaxed);
    }
    tl_telemetry.reset();
}

} // namespace runtime
} // namespace tachyon
