#include "tachyon/debug/crash_handler.h"
#include <iostream>
#include <string>
#include <ctime>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

namespace {
LONG WINAPI unhandled_exception_handler(EXCEPTION_POINTERS* exception_info) {
    char time_str[64];
    time_t now = time(nullptr);
    strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", localtime(&now));
    
    std::string filename = "tachyon_crash_" + std::string(time_str) + ".dmp";
    
    HANDLE file = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exception_info;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithFullMemory, &mdei, nullptr, nullptr);
        CloseHandle(file);
        
        std::cerr << "\n[CRASH] Tachyon has encountered a fatal error." << std::endl;
        std::cerr << "[CRASH] Minidump written to: " << filename << std::endl;
        std::cerr << "[CRASH] Please send this file to the developers for analysis." << std::endl;
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}
} // namespace
#else
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

namespace {
void signal_handler(int sig) {
    void* array[50];
    size_t size = backtrace(array, 50);

    std::cerr << "\n[CRASH] Tachyon has encountered a fatal error (Signal: " << sig << ")" << std::endl;
    std::cerr << "[CRASH] Backtrace:" << std::endl;
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    
    std::abort();
}
} // namespace
#endif

namespace tachyon::debug {

void setup_crash_handler() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(unhandled_exception_handler);
#else
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGFPE,  signal_handler);
    signal(SIGILL,  signal_handler);
#endif
}

} // namespace tachyon::debug
