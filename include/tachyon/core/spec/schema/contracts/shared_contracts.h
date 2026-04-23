#pragma once
#include "tachyon/core/animation/keyframe.h"

#include <string>
#include <vector>
#include <optional>
#include <utility>

namespace tachyon::spec {

/**
 * @brief Binding between a property and an external data source (e.g. tracker).
 */
struct TrackBinding {
    std::string property_path;
    std::string source_id;
    std::string source_track_name;
    float influence{1.0f};
    bool enabled{true};
};

/**
 * @brief Definition of a camera cut in the timeline.
 */
struct CameraCut {
    std::string camera_id;      ///< ID of the camera layer to activate.
    double start_seconds{0.0};   ///< Inclusive start time in seconds.
    double end_seconds{0.0};     ///< Exclusive end time in seconds.

    [[nodiscard]] bool contains(double time) const noexcept {
        return time >= start_seconds && time < end_seconds;
    }
};

/**
 * @brief Mode for evaluating source time from destination time.
 */
enum class TimeRemapMode {
    Hold,
    Blend,
    OpticalFlow
};

/**
 * @brief Temporal blending modes for time-remapped layers.
 */
enum class FrameBlendMode {
    None,           // No blending, nearest frame
    Linear,         // Cross-fade between frames
    PixelMotion,    // Use optical flow vectors
    OpticalFlow     // Full synthesis
};

/**
 * @brief Parameters for frame blending.
 */
struct FrameBlendParams {
    FrameBlendMode mode{FrameBlendMode::Linear};
    float blend_strength{1.0f}; // 0.0 = no blend, 1.0 = full blend
};

/**
 * @brief Parameters for motion blur temporal sampling.
 */
struct MotionBlurParams {
    float shutter_angle_deg{180.0f}; // 180° = half frame duration
    int sample_count{8};             // 8 for preview, 16+ for final
    uint32_t seed{0};                // for deterministic jitter
};

/**
 * @brief Modes for track matte compositing.
 */
enum class MatteMode {
    None,            ///< No matte applied.
    Alpha,           ///< Use source alpha as matte.
    AlphaInverted,   ///< Use inverted source alpha.
    Luma,            ///< Use source luminance as matte.
    LumaInverted     ///< Use inverted source luminance.
};

/**
 * @brief A matte dependency edge in the render graph.
 * 
 * The source layer provides the matte. The target layer consumes it.
 * Both IDs must resolve to valid layers during graph validation.
 * Cycles in matte dependencies are an error and must be caught at validation time.
 */
struct MatteDependency {
    std::string source_layer_id;  ///< Layer that provides the matte signal.
    std::string target_layer_id;  ///< Layer that consumes the matte signal.
    MatteMode mode{MatteMode::Alpha};
};

/**
 * @brief Time remap curve definition.
 */
struct TimeRemapCurve {
    std::vector<std::pair<float, float>> keyframes; // (source_time, dest_time)
    TimeRemapMode mode{TimeRemapMode::Blend};
    bool enabled{false};
};

struct AudioEffectSpec {
    std::string type; // "fade_in", "fade_out", "gain", "low_pass", "high_pass", "normalize"
    std::optional<double> start_time;
    std::optional<double> duration;
    std::optional<float> gain_db;
    std::optional<float> cutoff_freq_hz;
};

/**
 * @brief Specification for an audio track in the composition.
 */
struct AudioTrackSpec {
    std::string id;
    std::string source_path;
    float volume{1.0f};
    float pan{0.0f}; // -1.0 (left) to 1.0 (right)
    double start_offset_seconds{0.0};
    
    std::vector<animation::Keyframe<float>> volume_keyframes;
    std::vector<animation::Keyframe<float>> pan_keyframes;
    std::vector<AudioEffectSpec> effects;
};

} // namespace tachyon::spec
