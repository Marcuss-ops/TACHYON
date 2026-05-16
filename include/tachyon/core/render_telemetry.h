#pragma once

#include "tachyon/api.h"
#include <string>
#include <vector>
#include <chrono>

namespace tachyon {

/**
 * @brief Telemetry event data contract.
 * Fixed format for stable analysis (Excel, Pandas, etc.)
 */
struct TelemetryEvent {
    double ts = 0.0;           // Seconds since telemetry init
    int run_id = 0;
    std::string event;         // scene_render, frame_render, video_frame, video_export, image_render
    int frame = -1;
    int w = 0;
    int h = 0;
    double total_ms = 0.0;
    double setup_ms = 0.0;
    double composite_ms = 0.0;
    double blur_ms = 0.0;
    double encode_ms = 0.0;
    double ram_mb = 0.0;
    int cache_hit = 0;         // 1 if hit, 0 otherwise
    int layer_count = 0;
};

/**
 * @brief Centralized telemetry logger for the Tachyon engine.
 * Ensures a single point of writing and a stable data contract.
 */
class TACHYON_API RenderTelemetry {
public:
    static RenderTelemetry& get();

    /**
     * @brief Initialize telemetry with a CSV file and a summary file.
     * Calculates the next run_id by scanning the existing CSV.
     */
    void init(const std::string& csv_path = "render_telemetry.csv", 
              const std::string& summary_path = "render_summary.txt");

    /**
     * @brief Log a telemetry event to the CSV file.
     */
    void log(const TelemetryEvent& event);

    /**
     * @brief Save the human-readable summary for the current run.
     */
    void save_summary();

    /**
     * @brief Returns the current run ID.
     */
    int current_run_id() const { return m_run_id; }

private:
    RenderTelemetry() = default;
    
    std::string m_csv_path;
    std::string m_summary_path;
    int m_run_id = 0;
    std::chrono::steady_clock::time_point m_start_time;
    bool m_initialized = false;

    // Current run statistics
    struct RunStats {
        std::vector<double> frame_times;
        int total_cache_hits = 0;
        int total_frames = 0;
        double total_wall_time = 0.0;
        int max_layers = 0;
        
        // Phase totals for bottleneck detection
        double total_setup_ms = 0.0;
        double total_composite_ms = 0.0;
        double total_blur_ms = 0.0;
        double total_encode_ms = 0.0;
    } m_stats;

    void update_stats(const TelemetryEvent& event);
    int resolve_next_run_id();
};

} // namespace tachyon
