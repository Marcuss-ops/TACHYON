#include "tachyon/core/cli.h"
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    try {
        return tachyon::run_cli(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FATAL ERROR: Unknown exception" << std::endl;
        return 1;
    }
}
