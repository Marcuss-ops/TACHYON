#pragma once

#include "tachyon/runtime/diagnostics.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace tachyon {

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

struct OutputAudioProfile {
    std::string mode;
    std::string codec;
    std::optional<std::int64_t> sample_rate;
    std::optional<std::int64_t> channels;
};

struct OutputBufferingProfile {
    std::string strategy;
    std::optional<std::int64_t> max_frames_in_queue;
};

struct OutputColorProfile {
    std::string transfer;
    std::string range;
};

struct OutputProfile {
    std::string name;
    std::string class_name;
    std::string container;
    OutputVideoProfile video;
    OutputAudioProfile audio;
    OutputBufferingProfile buffering;
    OutputColorProfile color;
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
    std::string seed_policy_mode{"stable"};
    std::string compatibility_mode;
};

ParseResult<RenderJob> parse_render_job_json(const std::string& text);
ParseResult<RenderJob> parse_render_job_file(const std::filesystem::path& path);
ValidationResult validate_render_job(const RenderJob& job);

} // namespace tachyon
