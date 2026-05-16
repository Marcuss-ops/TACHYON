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

struct FontOptions {
    std::string family;
    std::vector<std::uint32_t> weights;
    std::vector<std::string> subsets;
    std::filesystem::path dest{"assets/fonts"};
    bool overwrite{false};
};

struct OutputPresetOptions {
    std::string command;
    std::string name;
};

struct ThumbOptions {
    std::optional<int> frame;
    std::filesystem::path output;
};

struct ProbeOptions {
    std::filesystem::path input;
    bool json_output{false};
};

struct ConcatOptions {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
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
    std::filesystem::path trace_json;

    // Domain options
    RenderOptions render;
    InspectOptions inspect;
    MetricsOptions metrics;
    
    FontOptions fonts;
    OutputPresetOptions output_presets;
    ThumbOptions thumb;
    ProbeOptions probe;
    ConcatOptions concat;
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
