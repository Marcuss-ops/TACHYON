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

} // namespace tachyon::output
