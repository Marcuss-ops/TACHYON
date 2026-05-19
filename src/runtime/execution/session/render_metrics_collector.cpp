#include "tachyon/runtime/execution/session/render_metrics_collector.h"
#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/core/render_telemetry.h"
#include "tachyon/output/output_utils.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace tachyon {

void RenderMetricsCollector::finalize_session(
    RenderSessionState& state,
    RenderSessionResult& result,
    ScopedProcessSampler& sampler
) {
    sampler.stop();
    const auto session_end = std::chrono::steady_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - state.session_start).count();
    result.peak_working_set_bytes = sampler.peak_working_set_bytes();
    result.avg_working_set_bytes = sampler.avg_working_set_bytes();
    result.peak_private_bytes = sampler.peak_private_bytes();
    result.avg_private_bytes = sampler.avg_private_bytes();
    result.avg_cpu_percent_machine = sampler.avg_cpu_percent_machine();
    result.avg_cpu_cores_used = sampler.avg_cpu_cores_used();

    const std::size_t total_frames = result.frames_written > 0 ? result.frames_written : result.frames.size();
    if (total_frames > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / static_cast<double>(total_frames);
    }
}

void RenderMetricsCollector::log_frame_events(
    const RenderSessionState& state,
    const RenderSessionResult& result
) {
    const bool is_video = output::output_requests_video_file(state.effective_plan.render_plan.output);
    std::uint32_t w = static_cast<std::uint32_t>(state.effective_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(state.effective_plan.render_plan.composition.height);

    for (size_t i = 0; i < result.frame_times_ms.size(); ++i) {
        TelemetryEvent fe;
        fe.event = is_video ? "video_frame" : "frame_render";
        fe.frame = static_cast<int>(state.effective_plan.frame_tasks[i].frame_number);
        fe.w = w;
        fe.h = h;
        fe.total_ms = result.frame_times_ms[i];
        RenderTelemetry::get().log(fe);
    }
}

void RenderMetricsCollector::persist_telemetry(
    const RenderSessionState& state,
    const RenderSessionResult& result
) {
    {
        RenderTelemetryRecord record;
        
        // Identity
        record.run_id = "session-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        record.job_id = "job-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        record.scene_id = state.resolved_output_path.empty() ? "anonymous_scene" : std::filesystem::path(state.resolved_output_path).stem().string();
        record.preset_id = "default";
        record.machine_id = "local";

        // Status
        record.success = (result.output_error.empty() && result.diagnostics.ok());
        if (!record.success) {
            record.error_code = "RenderFailed";
            record.error_message = result.output_error.empty() ? "Diagnostics error" : result.output_error;
        }

        // Output stats
        record.frames_total = static_cast<int>(state.effective_plan.frame_tasks.size());
        record.frames_written = static_cast<int>(result.frames_written);
        record.duration_seconds = static_cast<double>(record.frames_total) / 30.0; // fallback standard 30fps
        record.fps_target = 30.0;

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
        record.total_pixel_ops = result.total_pixel_ops;
        record.rasterized_pixels = result.rasterized_pixels;
        record.blend_pixel_ops = result.blend_pixel_ops;
        record.encoded_pixels = result.encoded_pixels;
        record.total_tiles = result.total_tiles;

        // Metadata
        record.output_path = state.resolved_output_path;
        
        // Finished Time
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
        record.finished_at_iso = ss.str();

        record.tachyon_version = "0.8.0-dev";
        
        TelemetryWriter::write_sqlite(record, result);
    }

    // Telemetry finalization
    TelemetryEvent se;
    se.event = "render_session_end";
    se.w = state.effective_plan.render_plan.output.profile.width.value_or(0);
    se.h = state.effective_plan.render_plan.output.profile.height.value_or(0);
    se.total_ms = result.wall_time_total_ms;
    se.setup_ms = result.io_read_ms;
    se.composite_ms = result.frame_execution_ms;
    se.encode_ms = result.encode_ms;
    RenderTelemetry::get().log(se);
}

} // namespace tachyon
