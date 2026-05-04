#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace tachyon {

struct SceneSpec;

struct OutputDestination {
    std::string path;
    bool overwrite{false};
};

struct OutputVideoProfile {
    std::string codec;
    std::string pixel_format;
    std::string rate_control_mode;
    std::optional<double> crf;
};

struct AudioTrack {
    std::string source_path;
    double volume{1.0};
    double start_offset_seconds{0.0};
};

struct OutputAudioProfile {
    std::string mode;
    std::string codec;
    std::optional<std::int64_t> sample_rate;
    std::optional<std::int64_t> channels;
    std::vector<AudioTrack> tracks;
};

struct OutputBufferingProfile {
    std::string strategy;
    std::optional<std::int64_t> max_frames_in_queue;
};

struct OutputColorProfile {
    std::string space;
    std::string transfer;
    std::string range;
};

enum class OutputFormat {
    Video,
    Gif,
    ImageSequence,
    ProRes,
    WebM
};

struct OutputProfile {
    std::string name;
    std::string class_name;
    std::string container;
    OutputFormat format{OutputFormat::Video};
    std::string alpha_mode;
    OutputVideoProfile video;
    OutputAudioProfile audio;
    OutputBufferingProfile buffering;
    OutputColorProfile color;
    std::optional<std::int64_t> width;
    std::optional<std::int64_t> height;
};

struct OutputContract {
    OutputDestination destination;
    OutputProfile profile;
};

struct FrameRange {
    std::int64_t start{0};
    std::int64_t end{0};
};

struct RenderJob {
    std::string job_id;
    std::string scene_ref;
    std::string composition_target;
    FrameRange frame_range;
    OutputContract output;
    std::string quality_tier{"high"};
    std::string compositing_alpha_mode{"premultiplied"};
    std::string working_space{"linear_rec709"};
    bool motion_blur_enabled{false};
    std::string motion_blur_mode{"camera_only"};
    std::int64_t motion_blur_samples{0};
    double motion_blur_shutter_angle{180.0};
    std::string motion_blur_curve{"box"};
    std::string seed_policy_mode{"stable"};
    std::string compatibility_mode;
    bool proxy_enabled{false};
    std::unordered_map<std::string, double> variables;
    std::unordered_map<std::string, std::string> string_variables;
    
    struct LayerOverride {
        std::optional<double> opacity;
        std::optional<bool> enabled;
        std::optional<std::string> text_content;
        std::optional<double> position_x;
        std::optional<double> position_y;
        std::optional<double> scale_x;
        std::optional<double> scale_y;
        std::optional<double> rotation;
    };
    std::unordered_map<std::string, LayerOverride> layer_overrides;
};

class RenderJobBuilder {
public:
    static RenderJob still_image(
        const std::string& composition_id, 
        std::int64_t frame, 
        const std::string& output_path);

    static RenderJob video_export(
        const std::string& composition_id, 
        FrameRange range, 
        const std::string& output_path);
};

ValidationResult validate_render_job(const RenderJob& job);
ValidationResult validate_render_preflight(const SceneSpec& scene, const RenderJob& job);
void apply_output_preset(OutputProfile& profile);

} // namespace tachyon
