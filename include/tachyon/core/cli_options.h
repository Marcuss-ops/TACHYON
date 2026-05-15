#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/jobs/render_job.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

struct RenderOptions {
    std::string quality{"draft"};
    std::optional<FrameRange> frame_range_override;
    std::filesystem::path output_override;
    std::optional<std::string> output_preset_id;
    bool render_all_compositions{false};
    
    // Preview specific
    std::optional<int> preview_frame_number;
    std::filesystem::path preview_output;
};

struct InspectOptions {
    std::filesystem::path job_path;
    bool json_output{false};
    int samples{5};
    bool include_info{false};
    bool samples_explicitly_set{false};
};

struct MetricsOptions {
    std::string subcommand;
    std::filesystem::path input;
    int top{20};
    double machine_cost_per_hour{0.0};
    bool json_output{false};
};

struct ToolOptions {
    // Fonts
    std::string font_family;
    std::vector<std::uint32_t> font_weights;
    std::vector<std::string> font_subsets;
    std::filesystem::path font_dest{"assets/fonts"};
    bool font_overwrite{false};
    
    // Output Presets
    std::string output_presets_command;
    std::string output_preset_name;
    
    // Thumb
    std::optional<int> thumb_frame;
};

struct MediaToolOptions {
    // Probe
    std::filesystem::path probe_input;
    bool json_output{false};
    
    // Concat
    std::vector<std::filesystem::path> concat_inputs;
    std::filesystem::path concat_output;
};

struct CliOptions {
    std::string command;
    bool show_version{false};
    
    // Core selection
    std::filesystem::path cpp_path;
    std::optional<std::string> preset_id;

    // Global overrides
    std::size_t worker_count{0};
    std::optional<std::size_t> memory_budget_bytes;
    std::filesystem::path library_path;
    std::filesystem::path output_dir;
    std::string output_format;
    std::optional<std::string> transition_id;

    // Domain options
    RenderOptions render;
    InspectOptions inspect;
    MetricsOptions metrics;
    ToolOptions tools;
    MediaToolOptions media;
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
