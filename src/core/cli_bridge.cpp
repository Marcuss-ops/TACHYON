#include "tachyon/core/cli.h"

namespace tachyon {

run_cli_func_t g_run_cli_ptr = nullptr;
init_backends_func_t g_init_backends_ptr = nullptr;

void register_cli_entrypoint(run_cli_func_t fn) {
    g_run_cli_ptr = fn;
}

void register_backend_init(init_backends_func_t fn) {
    g_init_backends_ptr = fn;
}

bool has_cli_entrypoint() {
    return g_run_cli_ptr != nullptr;
}

bool has_backend_init() {
    return g_init_backends_ptr != nullptr;
}

} // namespace tachyon
