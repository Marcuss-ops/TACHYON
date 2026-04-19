#pragma once

#include "tachyon/runtime/diagnostics.h"

#include <filesystem>
#include <string>

namespace tachyon {

struct CliOptions {
    std::string command;
    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::filesystem::path batch_path;
    std::filesystem::path output_override;
    std::size_t worker_count{1};
    bool json_output{false};
};

ParseResult<CliOptions> parse_cli_options(int argc, char** argv);

} // namespace tachyon
