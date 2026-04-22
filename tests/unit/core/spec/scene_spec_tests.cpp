#include <iostream>

bool run_scene_spec_parsing_tests();
bool run_scene_spec_validation_tests();
bool run_cli_tests();

bool run_scene_spec_tests() {
    bool all_ok = true;
    if (!run_scene_spec_parsing_tests()) {
        std::cerr << "Scene spec parsing tests failed\n";
        all_ok = false;
    }
    if (!run_scene_spec_validation_tests()) {
        std::cerr << "Scene spec validation tests failed\n";
        all_ok = false;
    }
    if (!run_cli_tests()) {
        std::cerr << "CLI tests failed\n";
        all_ok = false;
    }
    return all_ok;
}
