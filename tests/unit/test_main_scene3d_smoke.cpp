#include <iostream>

bool run_scene3d_smoke_tests();

int main() {
    if (!run_scene3d_smoke_tests()) {
        return 1;
    }

    std::cout << "All tests passed!\n";
    return 0;
}
