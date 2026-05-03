#include <iostream>

bool run_cli_tests();

bool run_scene_spec_tests() {
    bool all_ok = true;
    if (!run_cli_tests()) {
        std::cerr << "CLI tests failed\n";
        all_ok = false;
    }
    return all_ok;
}
