#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <random>
#include <optional>
#include <iomanip>
#include <cstring>

uint32_t g_test_seed = 0;

uint32_t get_global_random_seed() {
    return g_test_seed;
}

namespace {

struct TestCase {
    const char* name;
    bool (*fn)();
};

std::string_view get_env_var(const char* name) {
    if (const char* value = std::getenv(name); value && *value) {
        return value;
    }
    return {};
}

std::string_view test_filter() {
    return get_env_var("TACHYON_TEST_FILTER");
}

bool matches_filter(std::string_view name) {
    const std::string_view filter = test_filter();
    if (filter.empty()) {
        return true;
    }

    std::size_t start = 0;
    while (start <= filter.size()) {
        const std::size_t end = filter.find(',', start);
        const std::string_view token = filter.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
        if (!token.empty() && name.find(token) != std::string_view::npos) {
            return true;
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return false;
}

bool run_step(const char* name, bool (*fn)(), int iteration, int total_iterations) {
    if (!matches_filter(name)) {
        std::cerr << "[SKIP] " << name << '\n';
        return true;
    }

    std::cerr << "[RUN] " << name << '\n';
    const bool ok = fn();
    if (ok) {
        if (total_iterations > 1) {
            std::cerr << "[OK] " << name << " (" << (iteration + 1) << "/" << total_iterations << ")\n";
        } else {
            std::cerr << "[OK] " << name << '\n';
        }
    } else {
        if (total_iterations > 1) {
            std::cerr << "[FAIL] " << name << " (iteration " << (iteration + 1) << "/" << total_iterations << ")\n";
        } else {
            std::cerr << "[FAIL] " << name << '\n';
        }
    }
    return ok;
}

uint32_t initialize_seed() {
    std::string_view seed_str = get_env_var("TACHYON_TEST_SEED");
    if (!seed_str.empty()) {
        try {
            return static_cast<uint32_t>(std::stoul(std::string(seed_str)));
        } catch (...) {
        }
    }
    return std::random_device{}();
}

int get_repeat_count() {
    std::string_view repeat_str = get_env_var("TACHYON_TEST_REPEAT");
    if (!repeat_str.empty()) {
        try {
            return std::stoi(std::string(repeat_str));
        } catch (...) {
        }
    }
    return 1;
}

} // namespace

bool run_render_session_tests();
bool run_parallax_cards_tests();
bool run_png_3d_validation_tests();
bool run_motion_blur_tests();
bool run_time_remap_tests();
bool run_frame_blend_tests();
bool run_rolling_shutter_tests();
bool run_3d_modifier_tests();

int main(int argc, char** argv) {
    std::vector<TestCase> tests = {
        {"render_session", run_render_session_tests},
        {"parallax_cards", run_parallax_cards_tests},
        {"png_3d_validation", run_png_3d_validation_tests},
        {"motion_blur", run_motion_blur_tests},
        {"time_remap", run_time_remap_tests},
        {"frame_blend", run_frame_blend_tests},
        {"rolling_shutter", run_rolling_shutter_tests},
        {"three_d_modifier", run_3d_modifier_tests},
    };

    bool list_tests = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--list-tests") == 0 || std::strcmp(argv[i], "-l") == 0) {
            list_tests = true;
            break;
        }
    }

    if (list_tests || !get_env_var("TACHYON_LIST_TESTS").empty()) {
        std::cout << "Available tests:\n";
        std::size_t max_name_len = 0;
        for (const auto& test : tests) {
            max_name_len = std::max(max_name_len, std::strlen(test.name));
        }
        for (std::size_t i = 0; i < tests.size(); ++i) {
            std::cout << std::left << std::setw(max_name_len + 4) << tests[i].name;
            if ((i + 1) % 2 == 0) {
                std::cout << "\n";
            }
        }
        if (tests.size() % 2 != 0) {
            std::cout << "\n";
        }
        return 0;
    }

    g_test_seed = initialize_seed();
    const int repeat_count = get_repeat_count();

    for (int i = 0; i < repeat_count; ++i) {
        for (const auto& test : tests) {
            if (!run_step(test.name, test.fn, i, repeat_count)) {
                std::cerr << test.name << " tests failed\n";
                return 1;
            }
        }
    }

    if (repeat_count > 1) {
        std::cout << "All tests passed! (" << repeat_count << " iterations, seed=" << g_test_seed << ")\n";
    } else {
        std::cout << "All tests passed! (seed=" << g_test_seed << ")\n";
    }

    return 0;
}
