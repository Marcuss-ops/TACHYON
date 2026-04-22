#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include "tachyon/runtime/execution/jobs/render_job.h"

#include <filesystem>
#include <optional>
#include <string>

namespace tachyon {

struct CliOptions {
    std::string command;
    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::filesystem::path batch_path;
    std::filesystem::path output_override;
    std::size_t worker_count{1};
    std::optional<std::size_t> memory_budget_bytes;
    bool json_output{false};
    std::optional<FrameRange> frame_range_override;
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
