#include <iostream>

bool run_scene_spec_tests();
bool run_render_job_tests();
bool run_math_tests();
bool run_asset_resolution_tests();
bool run_framebuffer_tests();
bool run_rasterizer_tests();
bool run_surface_tests();
bool run_property_tests();
bool run_expression_tests();
bool run_text_tests();

int main() {
    if (!run_math_tests()) {
        std::cerr << "math tests failed\n";
        return 1;
    }

    if (!run_property_tests()) {
        std::cerr << "property tests failed\n";
        return 1;
    }

    if (!run_expression_tests()) {
        std::cerr << "expression tests failed\n";
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

    if (!run_rasterizer_tests()) {
        std::cerr << "rasterizer tests failed\n";
        return 1;
    }

    if (!run_surface_tests()) {
        std::cerr << "surface tests failed\n";
        return 1;
    }

    if (!run_text_tests()) {
        std::cerr << "text tests failed\n";
        return 1;
    }

    if (!run_scene_spec_tests()) {
        std::cerr << "scene spec tests failed\n";
        return 1;
    }

    if (!run_render_job_tests()) {
        std::cerr << "render job tests failed\n";
        return 1;
    }

    std::cout << "all tests passed\n";
    return 0;
}
