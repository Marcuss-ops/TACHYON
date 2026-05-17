#pragma once

namespace tachyon {

using run_cli_func_t = int(*)(int, char**);
extern run_cli_func_t g_run_cli_ptr;

using init_backends_func_t = void(*)(void);
extern init_backends_func_t g_init_backends_ptr;

int run_cli(int argc, char** argv);

} // namespace tachyon
