#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include <spdlog/fmt/fmt.h>
#include <fstream>
#include <filesystem>

#ifdef TACHYON_ENABLE_SQLITE_TELEMETRY
#include "tachyon/runtime/telemetry/sqlite_telemetry_store.h"
#include "tachyon/runtime/execution/session/render_session.h"
#endif

namespace tachyon {


std::mutex TelemetryWriter::s_write_mutex;

std::filesystem::path TelemetryWriter::get_default_directory() {
    const char* home = std::getenv("HOME");
    if (!home) home = std::getenv("USERPROFILE");
    if (!home) return ".tachyon/telemetry";

    std::filesystem::path path(home);
    path /= ".tachyon";
    path /= "telemetry";
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
    return path;
}

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 32) out += fmt::format("\\u{:04x}", c);
                else        out += static_cast<char>(c);
        }
    }
    return out;
}

static std::string record_to_json(const RenderTelemetryRecord& r) {
    return fmt::format(
        R"({{"run_id":"{}","job_id":"{}","scene_id":"{}","preset_id":"{}","machine_id":"{}","success":{},"error_code":"{}","error_message":"{}","frames_total":{},"frames_written":{},"wall_time_ms":{:.2f},"render_ms":{},"encode_ms":{},"effective_fps":{},"peak_working_set_bytes":{},"avg_working_set_bytes":{},"peak_private_bytes":{},"avg_private_bytes":{},"avg_cpu_percent_machine":{},"avg_cpu_cores_used":{},"output_path":"{}","finished_at_iso":"{}"}})",
        escape_json(r.run_id), escape_json(r.job_id), escape_json(r.scene_id),
        escape_json(r.preset_id), escape_json(r.machine_id),
        r.success ? "true" : "false",
        escape_json(r.error_code), escape_json(r.error_message),
        r.frames_total, r.frames_written,
        r.wall_time_ms, r.render_ms, r.encode_ms, r.effective_fps,
        r.peak_working_set_bytes, r.avg_working_set_bytes,
        r.peak_private_bytes, r.avg_private_bytes,
        r.avg_cpu_percent_machine, r.avg_cpu_cores_used,
        escape_json(r.output_path), escape_json(r.finished_at_iso)
    );
}

bool TelemetryWriter::append_jsonl(const RenderTelemetryRecord& record, const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(s_write_mutex);
    std::ofstream ofs(path, std::ios::app);
    if (!ofs) return false;
    ofs << record_to_json(record) << "\n";
    return true;
}

bool TelemetryWriter::append_tsv(const RenderTelemetryRecord& record, const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(s_write_mutex);
    bool exists = std::filesystem::exists(path);
    std::ofstream ofs(path, std::ios::app);
    if (!ofs) return false;

    if (!exists) {
        ofs << "run_id\tjob_id\tscene_id\tsuccess\twall_time_ms\teffective_fps\tpeak_mem\n";
    }
    ofs << fmt::format("{}\t{}\t{}\t{}\t{:.2f}\t{}\t{}\n",
                       record.run_id, record.job_id, record.scene_id,
                       record.success ? 1 : 0,
                       record.wall_time_ms, record.effective_fps,
                       record.peak_working_set_bytes);
    return true;
}

bool TelemetryWriter::write_batch_summary(const BatchTelemetrySummary& summary, const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) return false;

    ofs << fmt::format(
        "{{\n"
        "  \"batch_id\": \"{}\",\n"
        "  \"total_jobs\": {},\n"
        "  \"succeeded\": {},\n"
        "  \"failed\": {},\n"
        "  \"failure_rate_per_1000\": {},\n"
        "  \"avg_wall_time_ms\": {},\n"
        "  \"avg_effective_fps\": {},\n"
        "  \"peak_working_set_bytes\": {},\n"
        "  \"peak_private_bytes\": {},\n"
        "  \"estimated_total_cost_usd\": {}\n"
        "}}\n",
        escape_json(summary.batch_id),
        summary.total_jobs, summary.succeeded, summary.failed,
        summary.failure_rate_per_1000, summary.avg_wall_time_ms,
        summary.avg_effective_fps, summary.peak_working_set_bytes,
        summary.peak_private_bytes, summary.estimated_total_cost_usd
    );
    return true;
}

bool TelemetryWriter::write_sqlite(const RenderTelemetryRecord& record, const RenderSessionResult& session_result) {
#ifndef TACHYON_ENABLE_SQLITE_TELEMETRY
    (void)record;
    (void)session_result;
    return false;
#else
    std::lock_guard<std::mutex> lock(s_write_mutex);
    
    // Resolve default database path
    const auto db_dir = get_default_directory();
    const auto db_path = db_dir / "tachyon_render_history.sqlite";
    
    // Initialize the store
    SqliteTelemetryStore store;
    if (!store.initialize(db_path)) {
        return false;
    }
    
    // Write Render Run Record
    if (!store.write_render_record(record)) {
        return false;
    }
    
    // Write Frame Records
    std::vector<SqliteFrameRecord> frames;
    frames.reserve(session_result.frames.size());
    for (size_t i = 0; i < session_result.frames.size(); ++i) {
        SqliteFrameRecord f;
        f.frame_number = static_cast<int>(session_result.frames[i].frame_number);
        f.duration_ms = i < session_result.frame_times_ms.size() ? session_result.frame_times_ms[i] : 0.0;
        f.cache_hit = session_result.frames[i].cache_hit;
        f.encode_time_ms = session_result.frames.empty() ? 0.0 : (session_result.encode_ms / session_result.frames.size());
        f.write_time_ms = session_result.frames.empty() ? 0.0 : (session_result.io_write_ms / session_result.frames.size());
        frames.push_back(f);
    }
    if (!frames.empty()) {
        store.write_frame_records(record.run_id, frames);
    }
    
    // Write Phase Events
    std::vector<SqlitePhaseEventRecord> phases;
    if (session_result.scene_compile_ms > 0) {
        phases.push_back({"compile_scene", session_result.scene_compile_ms});
    }
    if (session_result.plan_build_ms > 0) {
        phases.push_back({"build_render_plan", session_result.plan_build_ms});
    }
    if (session_result.frame_execution_ms > 0) {
        phases.push_back({"rasterization", session_result.frame_execution_ms});
    }
    if (session_result.encode_ms > 0) {
        phases.push_back({"ffmpeg_encoding", session_result.encode_ms});
    }
    if (session_result.io_read_ms > 0) {
        phases.push_back({"io_read", session_result.io_read_ms});
    }
    if (session_result.io_write_ms > 0) {
        phases.push_back({"io_write", session_result.io_write_ms});
    }
    if (!phases.empty()) {
        store.write_phase_events(record.run_id, phases);
    }
    
    // Write Optimization Counters
    std::vector<SqliteCounterRecord> counters;
    counters.push_back({"cache_hits", static_cast<int64_t>(session_result.cache_hits)});
    counters.push_back({"cache_misses", static_cast<int64_t>(session_result.cache_misses)});
    if (!counters.empty()) {
        store.write_counters(record.run_id, counters);
    }
    
    return true;
#endif
}

} // namespace tachyon

