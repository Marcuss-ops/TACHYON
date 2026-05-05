#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include "tachyon/runtime/execution/jobs/render_job.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

struct CliOptions {
    std::string command;
    std::filesystem::path catalog_path;
    std::optional<std::string> transition_id;
    std::filesystem::path job_path;
    std::filesystem::path batch_path;
    std::filesystem::path output_override;
    std::filesystem::path output_dir;  // Common output directory for catalog-demo
    std::filesystem::path font_manifest_path;  // For inspect-fonts command
    std::size_t worker_count{1};
    std::optional<std::size_t> memory_budget_bytes;
    std::optional<FrameRange> frame_range_override;
    std::optional<int> preview_frame_number;  // For preview-frame command
    std::filesystem::path preview_output;     // Output PNG path for preview-frame
    std::optional<std::string> preset_id;
    std::filesystem::path cpp_path;           // Path to .cpp scene script
    std::string quality{"draft"};            // Rendering quality tier
    bool json_output{false};
    int inspect_samples{5};
    bool inspect_include_info{false};
    bool samples_explicitly_set{false};

    // fetch-fonts command options
    std::string font_family;
    std::vector<std::uint32_t> font_weights;
    std::vector<std::string> font_subsets;
    std::filesystem::path font_dest{"assets/fonts"};
    bool font_overwrite{false};
    // metrics command options
    std::string metrics_command;
    std::filesystem::path metrics_input;
    int metrics_top{20};
    double machine_cost_per_hour{0.0};

    bool show_version{false};
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
