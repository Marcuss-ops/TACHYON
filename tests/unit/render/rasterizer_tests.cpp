#include "tachyon/renderer2d/rasterizer.h"
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

bool run_rasterizer_tests() {
    using namespace tachyon::renderer2d;

    // Test 1: Solid Rectangle and Alpha Blending
    {
        Framebuffer fb(100, 100);
        fb.clear(Color::black());

        // Draw a blue background rect
        CPURasterizer::draw_rect(fb, {10, 10, 80, 80, Color::blue()});
        
        // Draw a semi-transparent red rect over it
        // Premultiplied Red (50% alpha) = (128, 0, 0, 128)
        CPURasterizer::draw_rect(fb, {30, 30, 40, 40, {128, 0, 0, 128}});

        Color blended = fb.get_pixel(50, 50);
        // Blending Red (128,0,0,128) over Blue (0,0,255,255)
        // R = 128 + 0 * (1 - 0.5) = 128
        // G = 0 + 0 * (1 - 0.5) = 0
        // B = 0 + 255 * (1 - 0.5) = 127/128
        check_true(blended.r == 128, "Alpha blending: red channel");
        check_true(blended.b >= 127 && blended.b <= 128, "Alpha blending: blue channel");
    }

    // Test 2: Clipping
    {
        Framebuffer fb(50, 50);
        fb.clear(Color::black());
        // Draw a rect that is partially outside
        CPURasterizer::draw_rect(fb, {-10, -10, 100, 100, Color::white()});
        
        check_true(fb.get_pixel(0, 0).r == 255, "Clipping: top-left inside");
        check_true(fb.get_pixel(49, 49).r == 255, "Clipping: bottom-right inside");
    }

    // Test 3: Lines
    {
        Framebuffer fb(100, 100);
        fb.clear(Color::black());
        CPURasterizer::draw_line(fb, {0, 0, 99, 99, Color::green()});
        
        check_true(fb.get_pixel(0, 0).g == 255, "Line: start point");
        check_true(fb.get_pixel(50, 50).g == 255, "Line: mid point");
        check_true(fb.get_pixel(99, 99).g == 255, "Line: end point");
    }

    // Visual Composition Test
    {
        Framebuffer fb(800, 450); // 16:9
        fb.clear({20, 25, 30, 255}); // Dark gray bg

        // Background grid-like rects
        for(int i=0; i<800; i+=100) {
             CPURasterizer::draw_line(fb, {i, 0, i, 449, {50, 55, 60, 255}});
        }

        // Draw some "WOW" shapes
        CPURasterizer::draw_rect(fb, {100, 100, 600, 250, {40, 45, 50, 255}}); // Card
        CPURasterizer::draw_line(fb, {100, 100, 700, 100, Color::white()}); // Top border
        
        // Translucent elements
        CPURasterizer::draw_rect(fb, {150, 150, 100, 100, {200, 100, 0, 150}}); // Orange-ish
        CPURasterizer::draw_rect(fb, {200, 200, 100, 100, {0, 150, 200, 150}}); // Blue-ish
        
        std::filesystem::path out_dir = "tests/output";
        std::filesystem::create_directories(out_dir);
        fb.save_png(out_dir / "raster_test_composition.png");
    }

    return g_failures == 0;
}
