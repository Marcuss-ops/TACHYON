#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>

namespace tachyon::core {

/**
 * @brief Measures execution time for individual pipeline stages.
 */
class StageTimer {
public:
    explicit StageTimer(const std::string& stage_name);
    ~StageTimer();

    void stop();

private:
    std::string stage_name;
    std::chrono::steady_clock::time_point start_time;
    bool stopped = false;
};

/**
 * @brief Singleton/Central registry for performance metrics and drift.
 */
class Profiler {
public:
    static Profiler& instance();

    void record_stage_time(const std::string& stage, double duration_ms);
    void record_bytes_processed(const std::string& component, size_t bytes);
    void record_frames_processed(int count);

    double get_stage_time(const std::string& stage) const;
    std::string generate_report() const;
    void reset();

private:
    Profiler() = default;
    std::unordered_map<std::string, double> stage_times;
    std::unordered_map<std::string, size_t> component_bytes;
    int total_frames = 0;
};

} // namespace tachyon::core
