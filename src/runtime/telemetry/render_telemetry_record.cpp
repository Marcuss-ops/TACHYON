#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/jobs/render_job.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace tachyon {

static std::string get_iso_8601_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

RenderTelemetryRecord make_render_telemetry_record(
    const RenderJob& job,
    const RenderSessionResult& result,
    const RenderTelemetryContext& context) 
{
    RenderTelemetryRecord record;
    
    // Identity
    record.run_id = context.run_id;
    record.job_id = job.job_id;
    record.scene_id = context.scene_id;
    record.preset_id = context.preset_id;
    record.machine_id = context.machine_id;

    // Status
    record.success = (result.output_error.empty());
    if (!record.success) {
        record.error_code = "RenderFailed"; // Default, should be refined by caller
        record.error_message = result.output_error;
    }

    // Output stats
    record.frames_total = context.frames_total > 0 ? context.frames_total : static_cast<int>(job.frame_range.end - job.frame_range.start);
    record.frames_written = static_cast<int>(result.frames_written);
    record.duration_seconds = context.duration_seconds;
    record.fps_target = context.fps_target;

    // Timings (ms)
    record.wall_time_ms = result.wall_time_total_ms;
    record.render_ms = result.frame_execution_ms;
    record.encode_ms = result.encode_ms;
    record.io_read_ms = result.io_read_ms;
    record.io_write_ms = result.io_write_ms;
    record.setup_ms = result.scene_compile_ms + result.plan_build_ms + result.execution_plan_build_ms;

    // Performance Metrics
    if (record.wall_time_ms > 0.0) {
        record.effective_fps = (static_cast<double>(record.frames_written) / (record.wall_time_ms / 1000.0));
        record.videos_per_hour = 3600000.0 / record.wall_time_ms;
    }
    
    if (record.duration_seconds > 0.0 && record.wall_time_ms > 0.0) {
        record.video_seconds_per_render_second = record.duration_seconds / (record.wall_time_ms / 1000.0);
    }

    // Resources
    record.peak_working_set_bytes = result.peak_working_set_bytes;
    record.avg_working_set_bytes = result.avg_working_set_bytes;
    record.peak_private_bytes = result.peak_private_bytes;
    record.avg_private_bytes = result.avg_private_bytes;
    record.avg_cpu_percent_machine = result.avg_cpu_percent_machine;
    record.avg_cpu_cores_used = result.avg_cpu_cores_used;

    // I/O
    record.input_bytes = result.input_bytes;
    record.output_bytes = result.output_bytes;

    record.cache_hit_rate = result.cache_hit_rate();

    // Metadata
    record.output_path = job.output.destination.path;
    // record.started_at_iso = ... // Set by caller or sampler start
    record.finished_at_iso = get_iso_8601_time();
    
    // Environment - these would ideally be populated once per run
    record.tachyon_version = "0.8.0-dev"; 

    return record;
}

} // namespace tachyon
