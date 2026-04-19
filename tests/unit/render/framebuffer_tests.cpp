#include "tachyon/renderer2d/framebuffer.h"

#include <iostream>
#include <string>
#include <filesystem>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_framebuffer_tests() {
    using namespace tachyon::renderer2d;

    // Test 1: Black frame
    {
        Framebuffer fb(100, 100);
        fb.clear(Color::black());
        
        bool all_black = true;
        for (uint32_t y = 0; y < 100; ++y) {
            for (uint32_t x = 0; x < 100; ++x) {
                Color c = fb.get_pixel(x, y);
                if (c.r != 0 || c.g != 0 || c.b != 0 || c.a != 255) {
                    all_black = false;
                    break;
                }
            }
        }
        check_true(all_black, "Framebuffer clear to black");
        
        std::filesystem::create_directories("tests/output");
        check_true(fb.save_png("tests/output/01_black.png"), "Save black PNG");
    }

    // Test 2: White pixel in center
    {
        Framebuffer fb(101, 101); // Odd size for perfect center
        fb.clear(Color::black());
        fb.set_pixel(50, 50, Color::white());
        
        Color center = fb.get_pixel(50, 50);
        check_true(center.r == 255 && center.g == 255 && center.b == 255, "White pixel at center");
        
        Color edge = fb.get_pixel(0, 0);
        check_true(edge.r == 0 && edge.g == 0 && edge.b == 0, "Edge still black");
        
        check_true(fb.save_png("tests/output/02_white_pixel.png"), "Save white pixel PNG");
    }

    // Test 3: Red rectangle
    {
        Framebuffer fb(200, 200);
        fb.clear(Color::blue()); // Start with blue background
        
        // Draw red rectangle from (50, 50) to (150, 150)
        for (uint32_t y = 50; y < 150; ++y) {
            for (uint32_t x = 50; x < 150; ++x) {
                fb.set_pixel(x, y, Color::red());
            }
        }
        
        check_true(fb.get_pixel(100, 100).r == 255, "Inside rectangle is red");
        check_true(fb.get_pixel(10, 10).b == 255, "Outside rectangle is blue");
        
        check_true(fb.save_png("tests/output/03_rectangle.png"), "Save rectangle PNG");
    }

    return g_failures == 0;
}
