#include "tachyon/core/c_api.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/diag/log.h"
#include "tachyon/backends/backend_init.h"
#include "tachyon/version.h"

#include <cstring>
#include <algorithm>
#include <mutex>
#include <string>
#include <vector>
#include <stdexcept>

namespace {

void write_error(char* buf, int size, const char* msg) {
    if (!buf || size <= 0 || !msg) return;
    int n = std::min<int>(size - 1, static_cast<int>(std::strlen(msg)));
    std::memcpy(buf, msg, static_cast<std::size_t>(n));
    buf[n] = '\0';
}

bool g_initialized = false;
std::once_flag g_init_flag;

} // namespace

extern "C" {

TACHYON_API const char* tachyon_version(void) {
    return TACHYON_VERSION_STR;
}

TACHYON_API int tachyon_init(void) {
    std::call_once(g_init_flag, []() {
        tachyon::diag::init_logging();
        tachyon::backends::initialize_all_backends();
        g_initialized = true;
    });
    return g_initialized ? 0 : 1;
}

TACHYON_API int tachyon_run(int argc, const char** argv,
                             char* error_out, int error_size) {
    if (argc <= 0 || !argv) {
        write_error(error_out, error_size, "tachyon_run: argc/argv invalid");
        return 1;
    }

    // run_cli takes char** — make mutable copies on the stack
    std::vector<std::string> args(argv, argv + argc);
    std::vector<char*> argv_mut;
    argv_mut.reserve(static_cast<std::size_t>(argc));
    for (auto& s : args) argv_mut.push_back(s.data());

    try {
        return tachyon::run_cli(static_cast<int>(argv_mut.size()), argv_mut.data());
    } catch (const std::exception& e) {
        write_error(error_out, error_size, e.what());
        return 1;
    } catch (...) {
        write_error(error_out, error_size, "unknown exception in tachyon_run");
        return 1;
    }
}

} // extern "C"
