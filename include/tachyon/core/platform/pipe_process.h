#pragma once
#include <cstdio>

namespace tachyon::core::platform {

inline FILE* open_pipe(const char* command, const char* mode) {
#ifdef _WIN32
    return _popen(command, mode);
#else
    return popen(command, mode);
#endif
}

inline FILE* open_write_pipe(const char* command) {
#ifdef _WIN32
    return open_pipe(command, "wb");
#else
    return open_pipe(command, "w");
#endif
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
