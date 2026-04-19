#include <iostream>

bool run_scene_spec_tests();
bool run_render_job_tests();
bool run_math_tests();
bool run_asset_resolution_tests();
bool run_framebuffer_tests();
bool run_property_tests();

int main() {
    if (!run_math_tests()) {
        std::cerr << "math tests failed\n";
        return 1;
    }

    if (!run_property_tests()) {
        std::cerr << "property tests failed\n";
        return 1;
    }

    if (!run_asset_resolution_tests()) {
        std::cerr << "asset resolution tests failed\n";
        return 1;
    }

    if (!run_framebuffer_tests()) {
        std::cerr << "framebuffer tests failed\n";
        return 1;
    }

    if (!run_scene_spec_tests()) {
        std::cerr << "tests failed\n";
        return 1;
    }

    if (!run_render_job_tests()) {
        std::cerr << "tests failed\n";
        return 1;
    }

    std::cout << "all tests passed\n";
    return 0;
}
