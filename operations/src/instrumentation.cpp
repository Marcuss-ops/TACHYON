#include "tachyon/core/render_graph/instrumentation.h"
#include <sstream>
#include <iomanip>

namespace tachyon::core {

StageTimer::StageTimer(const std::string& name) 
    : stage_name(name), start_time(std::chrono::steady_clock::now()) {}

StageTimer::~StageTimer() {
    stop();
}

void StageTimer::stop() {
    if (stopped) return;
    stopped = true;
    auto end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start_time).count();
    Profiler::instance().record_stage_time(stage_name, duration);
}

Profiler& Profiler::instance() {
    static Profiler p;
    return p;
}

void Profiler::record_stage_time(const std::string& stage, double duration_ms) {
    stage_times[stage] += duration_ms;
}

void Profiler::record_bytes_processed(const std::string& component, size_t bytes) {
    component_bytes[component] += bytes;
}

void Profiler::record_frames_processed(int count) {
    total_frames += count;
}

double Profiler::get_stage_time(const std::string& stage) const {
    auto it = stage_times.find(stage);
    return it != stage_times.end() ? it->second : 0.0;
}

std::string Profiler::generate_report() const {
    std::stringstream ss;
    ss << "--- TACHYON PROFILER REPORT ---\n";
    ss << std::left << std::setw(20) << "Stage" << "Time (ms)\n";
    ss << "-------------------------------\n";
    for (const auto& [stage, time] : stage_times) {
        ss << std::left << std::setw(20) << stage << std::fixed << std::setprecision(2) << time << "\n";
    }
    ss << "-------------------------------\n";
    ss << "Total Frames: " << total_frames << "\n";
    return ss.str();
}

void Profiler::reset() {
    stage_times.clear();
    component_bytes.clear();
    total_frames = 0;
}

} // namespace tachyon::core
