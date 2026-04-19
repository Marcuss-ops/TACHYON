#include <iostream>

bool run_scene_spec_tests();

int main() {
    if (!run_scene_spec_tests()) {
        std::cerr << "tests failed\n";
        return 1;
    }

    std::cout << "all tests passed\n";
    return 0;
}
