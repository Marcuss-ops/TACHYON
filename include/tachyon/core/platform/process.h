#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <cstdio>

namespace tachyon::core::platform {

struct ProcessSpec {
    std::filesystem::path executable;
    std::vector<std::string> args;
    std::optional<std::filesystem::path> working_directory;
    std::optional<std::chrono::milliseconds> timeout;
    bool capture_stderr{true};
};

struct ProcessResult {
    int exit_code{0};
    std::string output;
    std::string error;
    bool success{false};
    bool timed_out{false};
};

ProcessResult run_process(const ProcessSpec& spec);

inline FILE* open_pipe(const char* command, const char* mode) {
#ifdef _WIN32
    return _popen(command, mode);
#else
    return popen(command, mode);
#endif
}

inline FILE* open_write_pipe(const char* command) {
    return open_pipe(command, "wb");
}

inline FILE* open_read_pipe(const char* command) {
    return open_pipe(command, "r");
}

inline int close_pipe(FILE* f) {
#ifdef _WIN32
    return _pclose(f);
#else
    return pclose(f);
#endif
}

} // namespace tachyon::core::platform
