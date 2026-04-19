#include "tachyon/renderer2d/framebuffer.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_surface_tests() {
    using namespace tachyon::renderer2d;

    {
        SurfaceRGBA surface(8, 8);
        surface.clear({10, 20, 30, 255});
        const Color pixel = surface.get_pixel(3, 4);
        check_true(pixel.r == 10 && pixel.g == 20 && pixel.b == 30 && pixel.a == 255, "Clear color fills full surface");
    }

    {
        SurfaceRGBA surface(4, 4);
        surface.clear(Color::blue());
        surface.blend_pixel(1, 1, {128, 0, 0, 128});
        const Color pixel = surface.get_pixel(1, 1);
        check_true(pixel.r >= 92 && pixel.r <= 94, "Linear-light blend lifts red channel");
        check_true(pixel.b >= 187 && pixel.b <= 189, "Linear-light blend lifts blue channel");
        check_true(pixel.a == 255, "Alpha over output alpha");
    }

    {
        SurfaceRGBA surface(2, 2);
        surface.clear(Color::transparent());
        surface.blend_pixel(0, 0, {255, 0, 0, 128});
        const Color pixel = surface.get_pixel(0, 0);
        check_true(pixel.r == 255 && pixel.g == 0 && pixel.b == 0, "Half-transparent red stays straight after blend");
        check_true(pixel.a == 128, "Half-transparent red preserves alpha");
    }

    {
        SurfaceRGBA surface(10, 10);
        surface.clear(Color::transparent());
        surface.fill_rect({2, 3, 4, 2}, {0, 255, 0, 255}, false);
        check_true(surface.get_pixel(2, 3).g == 255, "Rect fill paints inside pixels");
        check_true(surface.get_pixel(5, 4).g == 255, "Rect fill reaches far edge inside rect");
        check_true(surface.get_pixel(1, 3).a == 0, "Rect fill preserves outside pixels");
    }

    {
        SurfaceRGBA surface(10, 10);
        surface.clear(Color::transparent());
        surface.set_clip_rect({4, 4, 3, 3});
        surface.fill_rect({2, 2, 6, 6}, {255, 255, 255, 255}, false);
        check_true(surface.get_pixel(4, 4).a == 255, "Clip rect keeps interior pixel");
        check_true(surface.get_pixel(6, 6).a == 255, "Clip rect keeps bottom-right interior pixel");
        check_true(surface.get_pixel(3, 4).a == 0, "Clip rect excludes left side");
        check_true(surface.get_pixel(7, 7).a == 0, "Clip rect excludes outside bottom-right");
    }

    {
        SurfaceRGBA surface(32, 16);
        surface.clear(Color::transparent());
        surface.fill_rect({4, 4, 12, 6}, {255, 255, 255, 255}, false);
        std::filesystem::create_directories("tests/output");
        check_true(surface.save_png("tests/output/05_surface_rect.png"), "Surface output PNG saves");
    }

    return g_failures == 0;
}
