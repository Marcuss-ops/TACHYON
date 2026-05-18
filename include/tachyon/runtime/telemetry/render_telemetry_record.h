#pragma once

#include "tachyon/api.h"
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <cmath>
#include <thread>

namespace tachyon {

struct RenderJob;
struct RenderSessionResult;

/**
 * @brief Standardized error codes for render failures.
 */
enum class TACHYON_API RenderErrorCode {
    None,
    SceneCompileFailed,
    PlanBuildFailed,
    MissingAsset,
    MissingFont,
    DecodeFailed,
    RenderFailed,
    EncodeFailed,
    OutputWriteFailed,
    AudioMuxFailed,
    Timeout,
    OutOfMemory,
    Cancelled,
    Unknown
};

TACHYON_API std::string to_string(RenderErrorCode code);

TACHYON_API bool is_retryable(RenderErrorCode code);

/**
 * @brief Zero-dependency logarithmic high-dynamic-range histogram for latency metrics.
 */
class TACHYON_API HighDynamicRangeHistogram {
public:
    static constexpr int BUCKET_COUNT = 128;
    std::vector<uint32_t> buckets{std::vector<uint32_t>(BUCKET_COUNT, 0)};

    void record_value(double ms) {
        if (ms <= 0.0) ms = 0.001; // Floor to 1 microsecond
        double us = ms * 1000.0;
        double log_val = std::log2(us);
        if (log_val < 0.0) log_val = 0.0;
        int index = static_cast<int>(log_val * (128.0 / 24.0));
        if (index < 0) index = 0;
        if (index >= BUCKET_COUNT) index = BUCKET_COUNT - 1;
        buckets[index]++;
    }

    double get_value_at_percentile(double percentile) const {
        uint64_t total = 0;
        for (auto count : buckets) total += count;
        if (total == 0) return 0.0;

        uint64_t target = static_cast<uint64_t>(std::ceil(total * percentile));
        uint64_t running_sum = 0;
        for (int i = 0; i < BUCKET_COUNT; i++) {
            running_sum += buckets[i];
            if (running_sum >= target) {
                double log_val = static_cast<double>(i) * (24.0 / 128.0);
                double us = std::pow(2.0, log_val);
                return us / 1000.0;
            }
        }
        return 0.0;
    }

    std::string serialize() const {
        std::string s;
        for (int i = 0; i < BUCKET_COUNT; ++i) {
            s += std::to_string(buckets[i]);
            if (i < BUCKET_COUNT - 1) s += ",";
        }
        return s;
    }

    static HighDynamicRangeHistogram deserialize(const std::string& str) {
        HighDynamicRangeHistogram hist;
        std::size_t start = 0;
        std::size_t end = str.find(',');
        int index = 0;
        while (end != std::string::npos && index < BUCKET_COUNT) {
            hist.buckets[index++] = std::stoul(str.substr(start, end - start));
            start = end + 1;
            end = str.find(',', start);
        }
        if (index < BUCKET_COUNT && start < str.size()) {
            hist.buckets[index] = std::stoul(str.substr(start));
        }
        return hist;
    }
};

/**
 * @brief Dynamic anomaly-preserving frame metrics sampler.
 */
class TACHYON_API AdaptiveSampler {
private:
    int m_frameCount = 0;
    double m_avgMs = 16.6;
public:
    bool should_sample(double frame_ms) {
        ++m_frameCount;
        m_avgMs = 0.9 * m_avgMs + 0.1 * frame_ms;
        if (m_frameCount < 100) return true; // Warmup
        if (frame_ms > 2.0 * m_avgMs) return true; // Anomaly Spike
        return (m_frameCount % 10) == 0; // 1 out of 10 sampling rate
    }
};

/**
 * @brief Contextual information for a render job that isn't known by the session itself.
 */
struct TACHYON_API RenderTelemetryContext {
    std::string run_id;
    std::string job_id;
    std::string scene_id;
    std::string preset_id;
    std::string machine_id;
    std::string output_path;
    
    // Distributed Tracing
    std::string trace_id;
    std::string parent_span_id;
    
    double fps_target{30.0};
    double duration_seconds{0.0};
    int frames_total{0};
};

/**
 * @brief Hardware environment snapshot collected once per session.
 */
struct TACHYON_API HardwareEnvironment {
    std::string cpu_model;              // "Intel(R) Core(TM) i7-10700K"
    int physical_cores{0};
    int logical_cores{0};
    double cpu_freq_mhz{0.0};           // Base frequency from CPU info
    std::string gpu_vendor;             // "NVIDIA Corporation"
    std::string gpu_driver;             // "R555_86 (Driver 560.94)"
    std::size_t total_ram_bytes{0};     // Total system RAM
    std::size_t total_vram_bytes{0};    // Total GPU VRAM (0 if unknown)
};

/**
 * @brief Build fingerprint for correlating failures to specific builds.
 */
struct TACHYON_API BuildFingerprint {
    std::string git_commit_short;       // 8-char git hash
    std::string build_type;             // "Release", "Debug", "RelWithDebInfo"
    std::string compiler_info;          // "clang-18", "msvc-19.4", "gcc-13"
};

/**
 * @brief Industrial-grade telemetry record for a single render job.
 */
struct TACHYON_API RenderTelemetryRecord {
    // Identity
    std::string run_id;
    std::string job_id;
    std::string scene_id;
    std::string preset_id;
    std::string machine_id;

    // Distributed Tracing
    std::string trace_id;
    std::string parent_span_id;

    // HDR Histogram
    std::string frame_time_hist;

    // Status
    bool success{false};
    std::string error_code;
    std::string error_message;
    std::string error_stage;

    // Output stats
    int frames_total{0};
    int frames_written{0};
    double duration_seconds{0.0};
    double fps_target{0.0};

    // Timings (ms)
    double wall_time_ms{0.0};
    double render_ms{0.0};
    double encode_ms{0.0};
    double io_read_ms{0.0};
    double io_write_ms{0.0};
    double setup_ms{0.0}; // compile + plan

    // Performance Metrics
    double effective_fps{0.0};
    double video_seconds_per_render_second{0.0};
    double videos_per_hour{0.0};

    // Resources
    std::size_t peak_working_set_bytes{0};
    std::size_t avg_working_set_bytes{0};
    std::size_t peak_private_bytes{0};
    std::size_t avg_private_bytes{0};

    double cpu_user_ms{0.0};
    double cpu_kernel_ms{0.0};
    double avg_cpu_percent_machine{0.0};
    double avg_cpu_cores_used{0.0};

    // I/O
    std::size_t input_bytes{0};
    std::size_t output_bytes{0};

    double cache_hit_rate{0.0};

    // Metadata
    std::string output_path;
    std::string started_at_iso;
    std::string finished_at_iso;
    
    // Environment
    std::string hostname;
    std::string os;
    std::string cpu_model;
    int logical_cores{0};
    std::string tachyon_version;

    // --- New diagnostic fields (v3) ---

    // Hardware Environment (item 1)
    int physical_cores{0};
    double cpu_freq_mhz{0.0};
    std::string gpu_vendor;
    std::string gpu_driver;
    std::size_t total_ram_bytes{0};
    std::size_t total_vram_bytes{0};

    // Build Fingerprint (item 2)
    std::string git_commit_short;
    std::string build_type;
    std::string compiler_info;

    // Failure Details (item 3)
    int exit_code{0};
    std::string error_category;

    // Detailed Metrics (item 4)
    std::size_t total_pixel_ops{0};
    std::size_t rasterized_pixels{0};
    std::size_t blend_pixel_ops{0};
    std::size_t encoded_pixels{0};
    int total_tiles{0};

    // Time Series (item 5) — comma-separated values
    std::string memory_samples;       // RSS in MB every 100ms
    std::string cpu_util_samples;     // CPU util % samples
    std::string gpu_util_samples;     // GPU util % samples

    // Preset Details (item 6)
    std::string preset_json;          // {"width":3840,"height":2160,"samples":256}

    // Key Performance Indicators (item 7)
    double time_to_first_frame_ms{0.0};
    int ffmpeg_queue_depth{0};
};

TACHYON_API RenderTelemetryRecord make_render_telemetry_record(
    const RenderJob& job,
    const RenderSessionResult& result,
    const RenderTelemetryContext& context);

/**
 * @brief Collect hardware environment information from the current system.
 * Reads /proc/cpuinfo, /proc/meminfo, and GPU driver info on Linux.
 */
TACHYON_API HardwareEnvironment collect_hardware_environment();

/**
 * @brief Collect build fingerprint using compile-time macros and git.
 */
TACHYON_API BuildFingerprint collect_build_fingerprint();

} // namespace tachyon
