#include "tachyon/core/platform/process.h"
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace tachyon::core::platform {

namespace {

std::string quote_shell_arg(const std::string& arg) {
    // Quote arguments containing spaces, escape internal quotes
    if (arg.find(' ') != std::string::npos || arg.empty()) {
        std::string quoted = "\"";
        for (char c : arg) {
            if (c == '\"') {
                quoted += "\\\"";
            } else {
                quoted += c;
            }
        }
        quoted += "\"";
        return quoted;
    }
    return arg;
}

std::string build_command_string(const ProcessSpec& spec) {
    std::stringstream ss;
    ss << quote_shell_arg(spec.executable.string());
    for (const auto& arg : spec.args) {
        ss << " " << quote_shell_arg(arg);
    }
    return ss.str();
}

} // anonymous namespace

#ifdef _WIN32

ProcessResult run_process(const ProcessSpec& spec) {
    ProcessResult result;
    std::string cmd = build_command_string(spec);

    // Use _popen to execute and capture output
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        result.error = "Failed to execute process: " + cmd;
        return result;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.output += buffer;
    }

    int exit_code = _pclose(pipe);
    result.exit_code = exit_code;
    result.success = (exit_code == 0);

    return result;
}

#else

ProcessResult run_process(const ProcessSpec& spec) {
    ProcessResult result;

    // Build argv array for execvp
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(spec.executable.c_str()));
    for (const auto& arg : spec.args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Create pipes for stdout/stderr
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        result.error = "Failed to create pipes";
        return result;
    }

    pid_t pid = fork();
    if (pid == -1) {
        result.error = "Failed to fork process";
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (spec.working_directory.has_value()) {
            chdir(spec.working_directory->c_str());
        }

        execvp(spec.executable.c_str(), argv.data());
        // If we get here, exec failed
        perror("execvp failed");
        _exit(127);
    }

    // Parent process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Read output
    char buffer[256];
    ssize_t bytes_read;
    while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        result.output += buffer;
    }

    // Read errors
    while ((bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        result.error += buffer;
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = -1;
    }

    result.success = (result.exit_code == 0);
    return result;
}

#endif

} // namespace tachyon::core::platform

