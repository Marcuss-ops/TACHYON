#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace tachyon::core::platform {

struct ProcessSpec {
    std::filesystem::path executable;
    std::vector<std::string> args;
    std::optional<std::filesystem::path> working_directory;
};

struct ProcessResult {
    int exit_code{0};
    std::string output;
    std::string error;
    bool success{false};
};

ProcessResult run_process(const ProcessSpec& spec);

} // namespace tachyon::core::platform
