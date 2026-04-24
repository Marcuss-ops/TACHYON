#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include "tachyon/runtime/execution/jobs/render_job.h"

#include <filesystem>
#include <optional>
#include <string>

namespace tachyon {

struct CliOptions {
    std::string command;
    std::filesystem::path library_path;
    std::optional<std::string> transition_id;
    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::filesystem::path batch_path;
    std::filesystem::path output_override;
    std::filesystem::path output_dir;  // Common output directory for studio-demo
    std::filesystem::path font_manifest_path;  // For inspect-fonts command
    std::size_t worker_count{1};
    std::optional<std::size_t> memory_budget_bytes;
    bool json_output{false};
    std::optional<FrameRange> frame_range_override;
    std::optional<int> preview_frame_number;  // For preview-frame command
    std::filesystem::path preview_output;     // Output PNG path for preview-frame
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
