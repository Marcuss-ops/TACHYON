#pragma once

namespace tachyon {

using run_cli_func_t = int(*)(int, char**);
extern run_cli_func_t g_run_cli_ptr;

using init_backends_func_t = void(*)(void);
extern init_backends_func_t g_init_backends_ptr;

void register_cli_entrypoint(run_cli_func_t fn);
void register_backend_init(init_backends_func_t fn);

bool has_cli_entrypoint();
bool has_backend_init();

int run_cli(int argc, char** argv);

} // namespace tachyon
