#include "tachyon/core/render_telemetry.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace tachyon {

RenderTelemetry& RenderTelemetry::get() {
    static RenderTelemetry instance;
    return instance;
}

void RenderTelemetry::init(const std::string& csv_path, const std::string& summary_path) {
    if (m_initialized) return;

    m_csv_path = csv_path;
    m_summary_path = summary_path;
    m_start_time = std::chrono::steady_clock::now();
    
    m_run_id = resolve_next_run_id();
    
    // Write header if file is new
    if (!std::filesystem::exists(m_csv_path) || std::filesystem::file_size(m_csv_path) == 0) {
        std::ofstream fs(m_csv_path);
        fs << "ts,run_id,event,frame,w,h,total_ms,setup_ms,composite_ms,blur_ms,encode_ms,ram_mb,cache_hit,layer_count\n";
    }

    m_initialized = true;
}

void RenderTelemetry::log(const TelemetryEvent& event) {
    if (!m_initialized) return;

    TelemetryEvent e = event;
    e.run_id = m_run_id;
    
    // Calculate timestamp if not provided
    if (e.ts == 0.0) {
        auto now = std::chrono::steady_clock::now();
        e.ts = std::chrono::duration<double>(now - m_start_time).count();
    }

    std::ofstream fs(m_csv_path, std::ios::app);
    fs << std::fixed << std::setprecision(3) << e.ts << ","
       << e.run_id << ","
       << e.event << ","
       << e.frame << ","
       << e.w << ","
       << e.h << ","
       << std::setprecision(2) << e.total_ms << ","
       << e.setup_ms << ","
       << e.composite_ms << ","
       << e.blur_ms << ","
       << e.encode_ms << ","
       << e.ram_mb << ","
       << e.cache_hit << ","
       << e.layer_count << "\n";

    update_stats(e);
}

void RenderTelemetry::update_stats(const TelemetryEvent& e) {
    if (e.event == "frame_render" || e.event == "video_frame") {
        m_stats.frame_times.push_back(e.total_ms);
        m_stats.total_cache_hits += e.cache_hit;
        m_stats.total_frames++;
        m_stats.max_layers = std::max(m_stats.max_layers, e.layer_count);
        
        m_stats.total_setup_ms += e.setup_ms;
        m_stats.total_composite_ms += e.composite_ms;
        m_stats.total_blur_ms += e.blur_ms;
        m_stats.total_encode_ms += e.encode_ms;
    } else if (e.event == "video_export" || e.event == "scene_render") {
        m_stats.total_wall_time = std::max(m_stats.total_wall_time, e.total_ms);
    }
}

void RenderTelemetry::save_summary() {
    if (!m_initialized || m_stats.total_frames == 0) return;

    std::vector<double> sorted = m_stats.frame_times;
    std::sort(sorted.begin(), sorted.end());

    auto get_p = [&](double p) {
        if (sorted.empty()) return 0.0;
        size_t idx = static_cast<size_t>(p * (sorted.size() - 1));
        return sorted[idx];
    };

    double p50 = get_p(0.5);
    double p95 = get_p(0.95);
    double avg = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();

    // Bottleneck detection
    std::string bottleneck = "unknown";
    double max_p = 0.0;
    if (m_stats.total_setup_ms > max_p) { max_p = m_stats.total_setup_ms; bottleneck = "setup"; }
    if (m_stats.total_composite_ms > max_p) { max_p = m_stats.total_composite_ms; bottleneck = "composite"; }
    if (m_stats.total_blur_ms > max_p) { max_p = m_stats.total_blur_ms; bottleneck = "blur"; }
    if (m_stats.total_encode_ms > max_p) { max_p = m_stats.total_encode_ms; bottleneck = "encode"; }

    double cache_rate = (static_cast<double>(m_stats.total_cache_hits) / m_stats.total_frames) * 100.0;

    std::ofstream fs(m_summary_path);
    fs << "Tachyon Render Summary - Run " << m_run_id << "\n";
    fs << "========================================\n";
    fs << "Frames:         " << m_stats.total_frames << "\n";
    fs << "Total Time:     " << std::fixed << std::setprecision(2) << m_stats.total_wall_time << " ms\n";
    fs << "Frame p50:      " << p50 << " ms\n";
    fs << "Frame p95:      " << p95 << " ms\n";
    fs << "Frame Avg:      " << avg << " ms\n";
    fs << "Cache Hit Rate: " << std::setprecision(1) << cache_rate << "%\n";
    fs << "Max Layers:     " << m_stats.max_layers << "\n";
    fs << "Bottleneck:     " << bottleneck << "\n";
    fs << "========================================\n";

    if (m_warmup_enabled) {
        fs << "\n--- WARMUP ---\n";
        fs << "warmup_enabled               : true\n";
        fs << "warmup_duration_ms           : " << std::fixed << std::setprecision(1) << m_warmup_duration_ms << "\n";
        fs << "warmup_buffers               : " << m_warmup_buffers << "\n";
        fs << "pool_bytes_after_warmup      : " << m_pool_bytes_after_warmup << "\n";
        fs << "pool_available_after_warmup  : " << m_pool_available_after_warmup << "\n";
    }

    if (m_static_bake_enabled) {
        fs << "\n--- STATIC BAKE ---\n";
        fs << "static_bake_enabled          : true\n";
        fs << "static_bake_hits             : " << m_static_bake_hits << "\n";
        fs << "static_bake_misses           : " << m_static_bake_misses << "\n";
        fs << "static_bake_bytes            : " << m_static_bake_bytes << "\n";
        fs << "static_bake_build_ms         : " << std::fixed << std::setprecision(1) << m_static_bake_build_ms << "\n";
    }
}

int RenderTelemetry::resolve_next_run_id() {
    if (!std::filesystem::exists(m_csv_path)) return 1;

    std::ifstream fs(m_csv_path);
    std::string line;
    int max_id = 0;

    // Skip header
    std::getline(fs, line);

    while (std::getline(fs, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string ts_str, id_str;
        std::getline(ss, ts_str, ','); // ts
        if (std::getline(ss, id_str, ',')) { // run_id
            try {
                int id = std::stoi(id_str);
                if (id > max_id) max_id = id;
            } catch (...) {}
        }
    }

    return max_id + 1;
}

} // namespace tachyon
