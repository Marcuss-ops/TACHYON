#pragma once

#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include <vector>
#include <map>

namespace tachyon {

/**
 * @brief Summary of telemetry records for a batch of jobs.
 */
struct BatchTelemetrySummary {
    std::string batch_id;
    int total_jobs{0};
    int succeeded{0};
    int failed{0};

    double failure_rate_per_1000{0.0};
    double total_wall_time_ms{0.0};
    double avg_wall_time_ms{0.0};
    double p50_wall_time_ms{0.0};
    double p95_wall_time_ms{0.0};

    double videos_per_hour{0.0};
    double avg_effective_fps{0.0};

    std::size_t peak_working_set_bytes{0};
    std::size_t peak_private_bytes{0};
    double avg_cpu_percent_machine{0.0};
    double avg_cpu_cores_used{0.0};

    double actual_batch_cost_per_video_usd{0.0};
    double estimated_isolated_job_cost_avg_usd{0.0};

    double machine_hourly_cost_usd{0.0};
    double estimated_total_cost_usd{0.0};
    double estimated_cost_per_video_usd{0.0};

    double total_encode_ms{0.0};
    double total_render_ms{0.0};
    double total_setup_ms{0.0};
    double total_io_read_ms{0.0};
    double total_io_write_ms{0.0};

    std::map<std::string, int> failures_by_error_code;
    std::map<std::string, int> failures_by_preset_id;
};

BatchTelemetrySummary aggregate_telemetry(const std::vector<RenderTelemetryRecord>& records, double machine_hourly_cost_usd = 0.0);

} // namespace tachyon
