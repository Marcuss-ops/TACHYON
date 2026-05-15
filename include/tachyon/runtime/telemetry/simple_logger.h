#pragma once
#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <mutex>
#include <filesystem>

namespace tachyon::runtime {

/**
 * @brief Simple render statistics for quick performance auditing.
 * Requested by user for manual review and p95 analysis.
 */
struct RenderStat {
    int64_t ts;
    double total_ms;
    double load_ms;
    double composite_ms;
    double blur_ms;
    double encode_ms;
    int w, h;
    std::string preset;
};

/**
 * @brief Thread-safe ring buffer logger for render performance metrics.
 * Keeps the last 100 entries in render_log.txt.
 */
class SimpleRenderLogger {
public:
    static SimpleRenderLogger& instance();

    void log(const RenderStat& stat);
    void save();

private:
    SimpleRenderLogger() = default;
    std::deque<RenderStat> m_history;
    static constexpr std::size_t MAX_ENTRIES = 100;
    std::mutex m_mutex;
};

} // namespace tachyon::runtime
