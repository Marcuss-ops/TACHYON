#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/jobs/render_job.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon {

/**
 * Core CLI options shared across most render-related commands.
 */
struct CliOptions {
    std::string command;
    std::filesystem::path cpp_path;           // Path to .cpp scene script
    std::optional<std::string> preset_id;
    
    std::filesystem::path output_override;
    std::optional<std::string> output_preset_id;
    std::filesystem::path output_dir;
    std::string output_format;
    
    std::size_t worker_count{0};
    std::optional<std::size_t> memory_budget_bytes;
    std::optional<FrameRange> frame_range_override;
    std::optional<int> preview_frame_number;
    std::filesystem::path preview_output;
    
    std::string quality{"draft"};
    bool render_all_compositions{false};
    bool json_output{false};
    bool show_version{false};

    // Extension options for specialized commands (Modularized)
    std::unordered_map<std::string, std::string> values;
    std::unordered_map<std::string, std::vector<std::string>> list_values;
    
    // Helper to get optional values
    std::optional<std::string> get_value(const std::string& key) const {
        auto it = values.find(key);
        if (it != values.end()) return it->second;
        return std::nullopt;
    }
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
