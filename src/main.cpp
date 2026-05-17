#include "tachyon/core/cli.h"
#include "tachyon/core/diag/log.h"
#include "tachyon/backends/backend_init.h"
#include <exception>

int main(int argc, char** argv) {
    tachyon::diag::init_logging();
    try {
        tachyon::backends::initialize_all_backends();
        return tachyon::run_cli(argc, argv);
    } catch (const std::exception& e) {
        tachyon::diag::critical("FATAL: {}", e.what());
        return 1;
    } catch (...) {
        tachyon::diag::critical("FATAL: unknown exception");
        return 1;
    }
}
