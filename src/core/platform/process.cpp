#include "tachyon/core/platform/process.h"
#include "tachyon/core/platform/shell_escape.h"
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
 
std::string build_command_string(const ProcessSpec& spec) {
    std::stringstream ss;
    ss << quote_shell_arg(spec.executable.string());
    for (std::size_t i = 0; i < spec.args.size(); ++i) {
        const auto& arg = spec.args[i];
#ifdef _WIN32
        // cmd.exe /C must receive the command string verbatim. Wrapping the
        // entire command in another layer of quotes breaks nested ffmpeg
        // quoting for paths.
        if (i == 1 &&
            (spec.executable.filename() == "cmd.exe" || spec.executable.filename() == "cmd") &&
            !spec.args.empty() &&
            (spec.args[0] == "/C" || spec.args[0] == "/c")) {
            ss << " " << arg;
            continue;
        }
#endif
        ss << " " << quote_shell_arg(arg);
    }
    return ss.str();
}

} // anonymous namespace

#ifdef _WIN32

ProcessResult run_process(const ProcessSpec& spec) {
    ProcessResult result;
    std::string cmd = build_command_string(spec);
    std::vector<char> cmd_buffer(cmd.begin(), cmd.end());
    cmd_buffer.push_back('\0');

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Rd = NULL;
    HANDLE hChildStd_ERR_Wr = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
        result.error = "Failed to create stdout pipe";
        return result;
    }
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) {
        result.error = "Failed to create stderr pipe";
        CloseHandle(hChildStd_OUT_Rd);
        CloseHandle(hChildStd_OUT_Wr);
        return result;
    }
    SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hChildStd_ERR_Wr;
    si.hStdOutput = hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    std::string current_dir = "";
    if (spec.working_directory.has_value()) {
        current_dir = spec.working_directory->string();
    }

    if (!CreateProcessA(
            NULL,
            cmd_buffer.data(),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            current_dir.empty() ? NULL : current_dir.c_str(),
            &si,
            &pi)) {
        result.error = "Failed to execute process: " + cmd + " (Error: " + std::to_string(GetLastError()) + ")";
        CloseHandle(hChildStd_OUT_Rd);
        CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_ERR_Rd);
        CloseHandle(hChildStd_ERR_Wr);
        return result;
    }

    // Close the write ends of the pipes in the parent process.
    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_ERR_Wr);

    DWORD dwRead;
    CHAR chBuf[4096];
    BOOL bSuccess = FALSE;

    // Read stdout
    for (;;) {
        bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;
        chBuf[dwRead] = '\0';
        result.output += chBuf;
    }

    // Read stderr
    for (;;) {
        bSuccess = ReadFile(hChildStd_ERR_Rd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;
        chBuf[dwRead] = '\0';
        result.error += chBuf;
    }

    CloseHandle(hChildStd_OUT_Rd);
    CloseHandle(hChildStd_ERR_Rd);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    result.exit_code = static_cast<int>(exit_code);
    result.success = (result.exit_code == 0);

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

