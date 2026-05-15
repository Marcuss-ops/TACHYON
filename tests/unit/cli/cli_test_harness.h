#pragma once

#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include <string>
#include <vector>
#include <sstream>

namespace tachyon::test {

struct CliTestResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
};

inline CliTestResult exec_cli(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    std::string prog = "tachyon";
    argv.push_back(prog.data());
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.data()));
    }
    
    // Note: This relies on run_cli being capture-able or we mock the streams
    // Since run_cli takes argc/argv, we might need a version that takes streams
    // Currently run_cli in src/cli/cli.cpp uses std::cout/std::cerr directly.
    // I should refactor run_cli to take streams for testability.
    
    return {0, "", ""}; // Placeholder for now
}

} // namespace tachyon::test
