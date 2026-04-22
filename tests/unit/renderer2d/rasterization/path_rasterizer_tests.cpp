#include "tachyon/renderer2d/raster/path_rasterizer.h"

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

bool run_path_rasterizer_tests() {
    using namespace tachyon::renderer2d;

    {
        SurfaceRGBA surface(32, 32);
        surface.clear(Color::transparent());

        PathGeometry path;
        path.commands = {
            {PathVerb::MoveTo, {4.0f, 4.0f}},
            {PathVerb::LineTo, {24.0f, 4.0f}},
            {PathVerb::LineTo, {24.0f, 24.0f}},
            {PathVerb::LineTo, {4.0f, 24.0f}},
            {PathVerb::Close}
        };

        FillPathStyle style;
        style.fill_color = Color::white();
        style.opacity = 1.0f;

        PathRasterizer::fill(surface, path, style);
        check_true(surface.get_pixel(8, 8).a > 0, "Filled path produces interior alpha");
        check_true(surface.get_pixel(1, 1).a == 0, "Filled path leaves exterior transparent");
    }

    {
        SurfaceRGBA surface(32, 32);
        surface.clear(Color::transparent());

        PathGeometry path;
        path.commands = {
            {PathVerb::MoveTo, {4.0f, 4.0f}},
            {PathVerb::LineTo, {24.0f, 4.0f}},
            {PathVerb::LineTo, {24.0f, 24.0f}},
            {PathVerb::LineTo, {4.0f, 24.0f}},
            {PathVerb::Close}
        };

        FillPathStyle style;
        style.fill_color = Color{1.0f, 0.0f, 0.0f, 0.5f};
        style.opacity = 0.5f;

        PathRasterizer::fill(surface, path, style);
        const Color pixel = surface.get_pixel(8, 8);
        check_true(pixel.a > 0, "Semi-transparent fill keeps alpha");
        check_true(pixel.r >= 0.99f, "Semi-transparent fill preserves source color");
        check_true(pixel.g == 0 && pixel.b == 0, "Semi-transparent fill preserves channel balance");
    }

    {
        SurfaceRGBA surface(32, 32);
        surface.clear(Color::transparent());

        PathGeometry path;
        path.commands = {
            {PathVerb::MoveTo, {4.0f, 16.0f}},
            {PathVerb::LineTo, {28.0f, 16.0f}}
        };

        StrokePathStyle style;
        style.stroke_color = Color::red();
        style.stroke_width = 3.0f;
        style.opacity = 1.0f;

        PathRasterizer::stroke(surface, path, style);
        check_true(surface.get_pixel(16, 16).a > 0, "Stroke path produces line coverage");
        check_true(surface.get_pixel(16, 4).a == 0, "Stroke stays localized");
    }

    return g_failures == 0;
}
