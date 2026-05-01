#pragma once
#include <cstdio>

namespace tachyon::core::platform {

inline FILE* open_write_pipe(const char* command) {
#ifdef _WIN32
    return _popen(command, "wb");
#else
    return popen(command, "wb");
#endif
}

inline int close_pipe(FILE* f) {
#ifdef _WIN32
    return _pclose(f);
#else
    return pclose(f);
#endif
}

} // namespace tachyon::core::platform
