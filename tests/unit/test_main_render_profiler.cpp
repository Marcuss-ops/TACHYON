#include <iostream>

namespace tachyon::profiling {
bool run_profiler_tests();
}

int main() {
    if (!tachyon::profiling::run_profiler_tests()) {
        return 1;
    }

    std::cout << "All tests passed!\n";
    return 0;
}
