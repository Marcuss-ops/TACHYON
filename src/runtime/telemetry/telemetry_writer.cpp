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
    if (const char* env_path = std::getenv("TACHYON_TELEMETRY_PATH")) {
        std::filesystem::path path(env_path);
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        return path;
    }

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
        R"({{"run_id":"{}","job_id":"{}","scene_id":"{}","preset_id":"{}","machine_id":"{}",)"
        R"("trace_id":"{}","parent_span_id":"{}",)"
        R"("success":{},"error_code":"{}","error_message":"{}","error_stage":"{}",)"
        R"("frames_total":{},"frames_written":{},"duration_seconds":{},"fps_target":{},)"
        R"("wall_time_ms":{:.2f},"render_ms":{},"encode_ms":{},"io_read_ms":{},"io_write_ms":{},"setup_ms":{},)"
        R"("effective_fps":{},"video_seconds_per_render_second":{},"videos_per_hour":{},)"
        R"("peak_working_set_bytes":{},"avg_working_set_bytes":{},"peak_private_bytes":{},"avg_private_bytes":{},)"
        R"("cpu_user_ms":{},"cpu_kernel_ms":{},"avg_cpu_percent_machine":{},"avg_cpu_cores_used":{},)"
        R"("input_bytes":{},"output_bytes":{},"cache_hit_rate":{},)"
        R"("output_path":"{}","started_at_iso":"{}","finished_at_iso":"{}",)"
        R"("hostname":"{}","os":"{}","cpu_model":"{}","logical_cores":{},"tachyon_version":"{}",)"
        R"("physical_cores":{},"cpu_freq_mhz":{},"gpu_vendor":"{}","gpu_driver":"{}","total_ram_bytes":{},"total_vram_bytes":{},)"
        R"("git_commit_short":"{}","build_type":"{}","compiler_info":"{}",)"
        R"("exit_code":{},"error_category":"{}",)"
        R"("total_pixel_ops":{},"rasterized_pixels":{},"blend_pixel_ops":{},"encoded_pixels":{},"total_tiles":{},)"
        R"("memory_samples":"{}","cpu_util_samples":"{}","gpu_util_samples":"{}",)"
        R"("preset_json":"{}",)"
        R"("time_to_first_frame_ms":{},"ffmpeg_queue_depth":{}}})",
        // Identity
        escape_json(r.run_id), escape_json(r.job_id), escape_json(r.scene_id),
        escape_json(r.preset_id), escape_json(r.machine_id),
        // Tracing
        escape_json(r.trace_id), escape_json(r.parent_span_id),
        // Status
        r.success ? "true" : "false",
        escape_json(r.error_code), escape_json(r.error_message), escape_json(r.error_stage),
        // Output stats
        r.frames_total, r.frames_written, r.duration_seconds, r.fps_target,
        // Timings (ms)
        r.wall_time_ms, r.render_ms, r.encode_ms, r.io_read_ms, r.io_write_ms, r.setup_ms,
        // Performance Metrics
        r.effective_fps, r.video_seconds_per_render_second, r.videos_per_hour,
        // Resources
        r.peak_working_set_bytes, r.avg_working_set_bytes,
        r.peak_private_bytes, r.avg_private_bytes,
        r.cpu_user_ms, r.cpu_kernel_ms, r.avg_cpu_percent_machine, r.avg_cpu_cores_used,
        // I/O
        r.input_bytes, r.output_bytes, r.cache_hit_rate,
        // Metadata
        escape_json(r.output_path), escape_json(r.started_at_iso), escape_json(r.finished_at_iso),
        // Environment (legacy)
        escape_json(r.hostname), escape_json(r.os), escape_json(r.cpu_model),
        r.logical_cores, escape_json(r.tachyon_version),
        // Hardware Environment (v3)
        r.physical_cores, r.cpu_freq_mhz,
        escape_json(r.gpu_vendor), escape_json(r.gpu_driver),
        r.total_ram_bytes, r.total_vram_bytes,
        // Build Fingerprint (v3)
        escape_json(r.git_commit_short), escape_json(r.build_type), escape_json(r.compiler_info),
        // Failure Details (v3)
        r.exit_code, escape_json(r.error_category),
        // Detailed Metrics (v3)
        r.total_pixel_ops, r.rasterized_pixels, r.blend_pixel_ops, r.encoded_pixels, r.total_tiles,
        // Time Series (v3)
        escape_json(r.memory_samples), escape_json(r.cpu_util_samples), escape_json(r.gpu_util_samples),
        // Preset Details (v3)
        escape_json(r.preset_json),
        // Key Performance Indicators (v3)
        r.time_to_first_frame_ms, r.ffmpeg_queue_depth
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
    
    // 1. Calculate logarithmic HDR Histogram & apply Adaptive Sampler filter
    HighDynamicRangeHistogram hist;
    AdaptiveSampler sampler;
    
    std::vector<SqliteFrameRecord> frames;
    frames.reserve(session_result.frames.size());
    for (size_t i = 0; i < session_result.frames.size(); ++i) {
        double duration = i < session_result.frame_times_ms.size() ? session_result.frame_times_ms[i] : 0.0;
        
        // Always populate histogram to capture complete profile
        hist.record_value(duration);
        
        // Conditionally sample frame records to optimize space
        if (sampler.should_sample(duration)) {
            SqliteFrameRecord f;
            f.frame_number = static_cast<int>(session_result.frames[i].frame_number);
            f.duration_ms = duration;
            f.cache_hit = session_result.frames[i].cache_hit;
            f.encode_time_ms = session_result.frames.empty() ? 0.0 : (session_result.encode_ms / session_result.frames.size());
            f.write_time_ms = session_result.frames.empty() ? 0.0 : (session_result.io_write_ms / session_result.frames.size());
            frames.push_back(f);
        }
    }
    
    // Enrich telemetry record with the serialized HDR histogram counts
    RenderTelemetryRecord enriched_record = record;
    enriched_record.frame_time_hist = hist.serialize();
    
    // Collect and attach hardware environment and build fingerprint
    HardwareEnvironment hw = collect_hardware_environment();
    enriched_record.cpu_model = hw.cpu_model;
    enriched_record.physical_cores = hw.physical_cores;
    enriched_record.logical_cores = hw.logical_cores;
    enriched_record.cpu_freq_mhz = hw.cpu_freq_mhz;
    enriched_record.gpu_vendor = hw.gpu_vendor;
    enriched_record.gpu_driver = hw.gpu_driver;
    enriched_record.total_ram_bytes = hw.total_ram_bytes;
    enriched_record.total_vram_bytes = hw.total_vram_bytes;
    
    BuildFingerprint build = collect_build_fingerprint();
    enriched_record.git_commit_short = build.git_commit_short;
    enriched_record.build_type = build.build_type;
    enriched_record.compiler_info = build.compiler_info;
    
    // Populate hostname and OS
    {
        std::ifstream hostfile("/proc/sys/kernel/hostname");
        if (hostfile) {
            std::getline(hostfile, enriched_record.hostname);
        } else {
            enriched_record.hostname = "unknown-host";
        }
    }
#if defined(__linux__)
    enriched_record.os = "Linux";
#elif defined(_WIN32)
    enriched_record.os = "Windows";
#elif defined(__APPLE__)
    enriched_record.os = "macOS";
#else
    enriched_record.os = "Unknown";
#endif
    
    // Write Render Run Record
    if (!store.write_render_record(enriched_record)) {
        return false;
    }
    
    // Write sampled frames
    if (!frames.empty()) {
        store.write_frame_records(enriched_record.run_id, frames);
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

    std::size_t node_cache_lookups = 0;
    std::size_t node_cache_hits = 0;
    std::size_t node_cache_misses = 0;
    std::size_t node_cache_bytes = 0;
    std::size_t static_nodes_detected = 0;
    std::size_t animated_nodes_detected = 0;

    for (const auto& fd : session_result.frame_diagnostics) {
        node_cache_lookups += fd.node_cache_lookups;
        node_cache_hits += fd.node_cache_hits;
        node_cache_misses += fd.node_cache_misses;
        static_nodes_detected += fd.static_nodes_detected;
        animated_nodes_detected += fd.animated_nodes_detected;
        if (fd.node_cache_bytes > node_cache_bytes) {
            node_cache_bytes = fd.node_cache_bytes;
        }
    }

    counters.push_back({"node_cache_lookups", static_cast<int64_t>(node_cache_lookups)});
    counters.push_back({"node_cache_hits", static_cast<int64_t>(node_cache_hits)});
    counters.push_back({"node_cache_misses", static_cast<int64_t>(node_cache_misses)});
    counters.push_back({"node_cache_bytes", static_cast<int64_t>(node_cache_bytes)});
    counters.push_back({"static_nodes_detected", static_cast<int64_t>(static_nodes_detected)});
    counters.push_back({"animated_nodes_detected", static_cast<int64_t>(animated_nodes_detected)});

    if (!counters.empty()) {
        store.write_counters(enriched_record.run_id, counters);
    }
    
    return true;
#endif
}

} // namespace tachyon

