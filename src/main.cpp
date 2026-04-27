#include "tachyon/core/cli.h"
#include "tachyon/debug/crash_handler.h"

int main(int argc, char** argv) {
    tachyon::debug::setup_crash_handler();
    return tachyon::run_cli(argc, argv);
}
