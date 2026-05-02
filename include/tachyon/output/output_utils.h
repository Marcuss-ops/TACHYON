#pragma once

#include "tachyon/runtime/execution/planning/render_plan.h"

namespace tachyon::output {

enum class OutputTargetKind {
    PngSequence,
    VideoFile,
    ExrSequence,
    Unknown
};

OutputTargetKind classify_output_contract(const OutputContract& contract);

bool output_requests_png_sequence(const OutputContract& contract);
bool output_requests_video_file(const OutputContract& contract);
bool output_requests_exr(const OutputContract& contract);

std::string build_ffmpeg_video_command(const RenderPlan& plan, const std::filesystem::path& output_path, bool include_faststart = true);
std::string build_ffmpeg_mux_command(const RenderPlan& plan, const std::filesystem::path& video_path, const std::filesystem::path& audio_path);

} // namespace tachyon::output
