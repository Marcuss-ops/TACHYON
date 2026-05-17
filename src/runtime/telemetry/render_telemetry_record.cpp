#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/jobs/render_job.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <fstream>
#include <thread>


namespace tachyon {

std::string to_string(RenderErrorCode code) {
    switch (code) {
        case RenderErrorCode::None: return "None";
        case RenderErrorCode::SceneCompileFailed: return "SceneCompileFailed";
        case RenderErrorCode::PlanBuildFailed: return "PlanBuildFailed";
        case RenderErrorCode::MissingAsset: return "MissingAsset";
        case RenderErrorCode::MissingFont: return "MissingFont";
        case RenderErrorCode::DecodeFailed: return "DecodeFailed";
        case RenderErrorCode::RenderFailed: return "RenderFailed";
        case RenderErrorCode::EncodeFailed: return "EncodeFailed";
        case RenderErrorCode::OutputWriteFailed: return "OutputWriteFailed";
        case RenderErrorCode::AudioMuxFailed: return "AudioMuxFailed";
        case RenderErrorCode::Timeout: return "Timeout";
        case RenderErrorCode::OutOfMemory: return "OutOfMemory";
        case RenderErrorCode::Cancelled: return "Cancelled";
        case RenderErrorCode::Unknown:
        default: return "Unknown";
    }
}

bool is_retryable(RenderErrorCode code) {
    switch (code) {
        case RenderErrorCode::EncodeFailed:
        case RenderErrorCode::OutputWriteFailed:
        case RenderErrorCode::AudioMuxFailed:
        case RenderErrorCode::Timeout:
        case RenderErrorCode::Unknown:
            return true;
        
        case RenderErrorCode::SceneCompileFailed:
        case RenderErrorCode::PlanBuildFailed:
        case RenderErrorCode::MissingAsset:
        case RenderErrorCode::MissingFont:
        case RenderErrorCode::OutOfMemory:
        case RenderErrorCode::Cancelled:
        case RenderErrorCode::None:
            return false;
            
        default:
            return false;
    }
}

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

    // Tracing Correlation Setup
    if (!context.trace_id.empty()) {
        record.trace_id = context.trace_id;
    } else {
        std::mt19937_64 rng(std::random_device{}());
        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << rng();
        record.trace_id = ss.str();
    }

    if (!context.parent_span_id.empty()) {
        record.parent_span_id = context.parent_span_id;
    } else {
        record.parent_span_id = "root";
    }

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

HardwareEnvironment collect_hardware_environment() {
    HardwareEnvironment env;

    // CPU: read /proc/cpuinfo for model name and frequency
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo) {
        std::string line;
        int cpu_count = 0;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                if (env.cpu_model.empty()) {
                    auto colon = line.find(':');
                    if (colon != std::string::npos) {
                        env.cpu_model = line.substr(colon + 2);
                    }
                }
                cpu_count++;
            }
            if (line.find("cpu MHz") != std::string::npos && env.cpu_freq_mhz == 0.0) {
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    env.cpu_freq_mhz = std::stod(line.substr(colon + 2));
                }
            }
            if (line.find("cpu cores") != std::string::npos && env.physical_cores == 0) {
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    env.physical_cores = std::stoi(line.substr(colon + 2));
                }
            }
        }
        env.logical_cores = cpu_count;
        // Fallback if cpu cores wasn't found
        if (env.physical_cores == 0) {
            env.physical_cores = env.logical_cores / 2;
            if (env.physical_cores < 1) env.physical_cores = env.logical_cores;
        }
    }

    // Fallback: use std::thread::hardware_concurrency()
    if (env.logical_cores == 0) {
        unsigned int hc = std::thread::hardware_concurrency();
        env.logical_cores = static_cast<int>(hc);
        env.physical_cores = env.logical_cores / 2;
        if (env.physical_cores < 1) env.physical_cores = env.logical_cores;
    }

    // RAM: read /proc/meminfo for MemTotal
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                // Format: "MemTotal:       16384000 kB"
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string val_str = line.substr(colon + 1);
                    // Trim and parse kB value
                    val_str.erase(0, val_str.find_first_not_of(" \t"));
                    std::size_t kb_pos = val_str.find("kB");
                    if (kb_pos != std::string::npos) {
                        val_str = val_str.substr(0, kb_pos);
                        val_str.erase(val_str.find_last_not_of(" \t") + 1);
                        long long kb = std::stoll(val_str);
                        env.total_ram_bytes = static_cast<std::size_t>(kb * 1024);
                    }
                }
                break;
            }
        }
    }

    // GPU: Try to read NVIDIA driver info
    std::ifstream nvidia_ver("/proc/driver/nvidia/version");
    if (nvidia_ver) {
        env.gpu_vendor = "NVIDIA Corporation";
        std::string line;
        if (std::getline(nvidia_ver, line)) {
            // Line: "NVRM version: NVIDIA UNIX x86_64 Kernel Module  560.94  Tue..."
            auto ver_pos = line.find("Module  ");
            if (ver_pos != std::string::npos) {
                env.gpu_driver = line.substr(ver_pos + 8);
                auto space = env.gpu_driver.find(' ');
                if (space != std::string::npos) {
                    env.gpu_driver = env.gpu_driver.substr(0, space);
                }
            }
        }
    }

    // Try to get GPU name from /sys/class/drm/
    // Also try lspci as fallback for GPU detection
    // We'll just note the vendor from /proc/driver if available
    if (env.gpu_vendor.empty()) {
        // Check for AMD GPU
        std::ifstream amd_ver("/sys/class/drm/version");
        // If nothing works, leave as empty string
    }

    // Try to read VRAM from NVIDIA
    // nvidia-smi would be ideal but we can try /sys
    // For now, vram stays 0 if we can't detect

    return env;
}

BuildFingerprint collect_build_fingerprint() {
    BuildFingerprint fp;

    // Git commit — try GIT_VERSION macro, else read .git/HEAD
    // We'll use a compile-time approach
#if defined(TACHYON_GIT_COMMIT)
    fp.git_commit_short = TACHYON_GIT_COMMIT;
#else
    // Try to read from .git/HEAD at runtime as fallback
    std::ifstream git_head(".git/HEAD");
    if (git_head) {
        std::string ref;
        if (std::getline(git_head, ref)) {
            if (ref.find("ref:") != std::string::npos) {
                auto space = ref.rfind(' ');
                if (space != std::string::npos) {
                    std::string ref_path = ".git/" + ref.substr(space + 1);
                    ref_path.erase(ref_path.find_last_not_of(" \t\n\r") + 1);
                    std::ifstream git_ref(ref_path);
                    if (git_ref) {
                        std::getline(git_ref, fp.git_commit_short);
                    }
                }
            } else {
                fp.git_commit_short = ref;
            }
        }
        if (!fp.git_commit_short.empty()) {
            fp.git_commit_short = fp.git_commit_short.substr(0, 8);
        }
    }
#endif

    // Build type from CMake
#if defined(NDEBUG)
    fp.build_type = "Release";
#elif defined(_DEBUG)
    fp.build_type = "Debug";
#else
    fp.build_type = "RelWithDebInfo";
#endif

    // Compiler info
#if defined(__clang__)
    fp.compiler_info = "clang-";
    fp.compiler_info += std::to_string(__clang_major__);
#elif defined(__GNUC__)
    fp.compiler_info = "gcc-";
    fp.compiler_info += std::to_string(__GNUC__);
#elif defined(_MSC_VER)
    fp.compiler_info = "msvc-";
    fp.compiler_info += std::to_string(_MSC_VER);
#else
    fp.compiler_info = "unknown";
#endif

    return fp;
}

} // namespace tachyon
