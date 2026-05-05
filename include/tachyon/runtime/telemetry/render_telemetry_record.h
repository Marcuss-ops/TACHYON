#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace tachyon {

struct RenderJob;
struct RenderSessionResult;

/**
 * @brief Standardized error codes for render failures.
 */
enum class RenderErrorCode {
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

inline std::string to_string(RenderErrorCode code) {
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

inline bool is_retryable(RenderErrorCode code) {
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

/**
 * @brief Contextual information for a render job that isn't known by the session itself.
 */
struct RenderTelemetryContext {
    std::string run_id;
    std::string job_id;
    std::string scene_id;
    std::string preset_id;
    std::string machine_id;
    std::string output_path;
    
    double fps_target{30.0};
    double duration_seconds{0.0};
    int frames_total{0};
};

/**
 * @brief Industrial-grade telemetry record for a single render job.
 */
struct RenderTelemetryRecord {
    // Identity
    std::string run_id;
    std::string job_id;
    std::string scene_id;
    std::string preset_id;
    std::string machine_id;

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
};

RenderTelemetryRecord make_render_telemetry_record(
    const RenderJob& job,
    const RenderSessionResult& result,
    const RenderTelemetryContext& context);

} // namespace tachyon
