#include <iostream>

namespace tachyon {
bool run_native_render_tests();
}

int main() {
    if (!tachyon::run_native_render_tests()) {
        std::cerr << "native_render tests failed\n";
        return 1;
    }
    std::cout << "All tests passed!\n";
    return 0;
}
