#include "tachyon/runtime/telemetry/batch_telemetry_aggregator.h"
#include <algorithm>
#include <numeric>

namespace tachyon {

BatchTelemetrySummary aggregate_telemetry(const std::vector<RenderTelemetryRecord>& records, double machine_hourly_cost_usd) {
    BatchTelemetrySummary summary;
    if (records.empty()) return summary;

    summary.batch_id = records.front().run_id;
    summary.total_jobs = static_cast<int>(records.size());
    summary.machine_hourly_cost_usd = machine_hourly_cost_usd;

    std::vector<double> wall_times;
    std::vector<double> effective_fps_values;
    std::vector<std::size_t> ws_memory_values;
    std::vector<std::size_t> private_memory_values;
    std::vector<double> cpu_machine_values;
    std::vector<double> cpu_cores_values;

    for (const auto& r : records) {
        if (r.success) {
            summary.succeeded++;
            wall_times.push_back(r.wall_time_ms);
            effective_fps_values.push_back(r.effective_fps);
            
            summary.total_render_ms += r.render_ms;
            summary.total_encode_ms += r.encode_ms;
            summary.total_setup_ms += r.setup_ms;
            summary.total_io_read_ms += r.io_read_ms;
            summary.total_io_write_ms += r.io_write_ms;
        } else {
            summary.failed++;
            summary.failures_by_error_code[r.error_code]++;
        }
        
        summary.failures_by_preset_id[r.preset_id]++;
        
        ws_memory_values.push_back(r.peak_working_set_bytes);
        private_memory_values.push_back(r.peak_private_bytes);
        if (r.avg_cpu_percent_machine > 0) cpu_machine_values.push_back(r.avg_cpu_percent_machine);
        if (r.avg_cpu_cores_used > 0) cpu_cores_values.push_back(r.avg_cpu_cores_used);
        
        summary.peak_working_set_bytes = std::max(summary.peak_working_set_bytes, r.peak_working_set_bytes);
        summary.peak_private_bytes = std::max(summary.peak_private_bytes, r.peak_private_bytes);
    }

    if (summary.total_jobs > 0) {
        summary.failure_rate_per_1000 = (static_cast<double>(summary.failed) / summary.total_jobs) * 1000.0;
    }

    if (!wall_times.empty()) {
        summary.total_wall_time_ms = std::accumulate(wall_times.begin(), wall_times.end(), 0.0);
        summary.avg_wall_time_ms = summary.total_wall_time_ms / wall_times.size();
        
        std::vector<double> sorted_wall = wall_times;
        std::sort(sorted_wall.begin(), sorted_wall.end());
        summary.p50_wall_time_ms = sorted_wall[sorted_wall.size() / 2];
        summary.p95_wall_time_ms = sorted_wall[static_cast<size_t>(sorted_wall.size() * 0.95)];
        
        summary.videos_per_hour = (summary.succeeded > 0) ? (3600000.0 / summary.avg_wall_time_ms) : 0.0;
    }

    if (!effective_fps_values.empty()) {
        summary.avg_effective_fps = std::accumulate(effective_fps_values.begin(), effective_fps_values.end(), 0.0) / effective_fps_values.size();
    }

    if (!cpu_machine_values.empty()) {
        summary.avg_cpu_percent_machine = std::accumulate(cpu_machine_values.begin(), cpu_machine_values.end(), 0.0) / cpu_machine_values.size();
    }
    
    if (!cpu_cores_values.empty()) {
        summary.avg_cpu_cores_used = std::accumulate(cpu_cores_values.begin(), cpu_cores_values.end(), 0.0) / cpu_cores_values.size();
    }

    // Cost calculations
    if (summary.machine_hourly_cost_usd > 0.0) {
        // Isolated cost (sum of efforts)
        summary.estimated_total_cost_usd = (summary.total_wall_time_ms / 3600000.0) * summary.machine_hourly_cost_usd;
        if (summary.succeeded > 0) {
            summary.estimated_isolated_job_cost_avg_usd = summary.estimated_total_cost_usd / summary.succeeded;
        }
        
        // Actual batch cost (wall clock of the machine)
        // Since we don't have the true batch wall clock here easily without parsing ISO dates, 
        // we'll assume for now this will be populated by a higher-level tool or use a simple heuristic if concurrency is known.
        // For now, let's just use the effort-based one as a baseline.
    }

    return summary;
}

} // namespace tachyon
