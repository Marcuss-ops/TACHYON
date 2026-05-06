#pragma once

#include <string_view>
#include <vector>
#include <cstdint>
#include <iostream>

namespace tachyon::test {

struct TestCase {
    const char* name;
    bool (*fn)();
};

uint32_t get_global_random_seed();

int run_test_suite(int argc, char** argv, const std::vector<TestCase>& tests);

} // namespace tachyon::test
