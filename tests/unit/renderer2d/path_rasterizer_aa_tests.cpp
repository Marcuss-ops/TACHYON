#include "tachyon/renderer2d/path_rasterizer.h"

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

bool run_path_rasterizer_aa_tests() {
    using namespace tachyon::renderer2d;

    g_failures = 0;

    SurfaceRGBA surface(32, 32);
    surface.clear(Color::transparent());

    PathGeometry path;
    path.commands = {
        {PathVerb::MoveTo, {4.25f, 4.25f}},
        {PathVerb::LineTo, {24.25f, 4.25f}},
        {PathVerb::LineTo, {24.25f, 24.25f}},
        {PathVerb::LineTo, {4.25f, 24.25f}},
        {PathVerb::Close}
    };

    FillPathStyle fill_style;
    fill_style.fill_color = Color::white();
    fill_style.opacity = 1.0f;
    fill_style.winding = WindingRule::EvenOdd;

    PathRasterizer::fill(surface, path, fill_style);

    const Color interior = surface.get_pixel(8, 8);
    const Color edge = surface.get_pixel(4, 8);
    const Color outside = surface.get_pixel(2, 2);

    check_true(interior.a == 255, "interior pixel should stay opaque");
    check_true(edge.a > 0 && edge.a < 255, "edge pixel should have partial coverage");
    check_true(outside.a == 0, "outside pixel should stay transparent");

    SurfaceRGBA stroke_surface(32, 32);
    stroke_surface.clear(Color::transparent());

    PathGeometry stroke_path;
    stroke_path.commands = {
        {PathVerb::MoveTo, {4.25f, 16.25f}},
        {PathVerb::LineTo, {24.25f, 16.25f}}
    };

    StrokePathStyle stroke_style;
    stroke_style.stroke_color = Color::red();
    stroke_style.stroke_width = 2.0f;
    stroke_style.opacity = 1.0f;

    PathRasterizer::stroke(stroke_surface, stroke_path, stroke_style);

    const Color stroke_center = stroke_surface.get_pixel(16, 16);
    const Color stroke_edge = stroke_surface.get_pixel(16, 15);
    const Color stroke_outside = stroke_surface.get_pixel(16, 10);

    check_true(stroke_center.a > 0, "stroke center should receive coverage");
    check_true(stroke_edge.a > 0 && stroke_edge.a < 255, "stroke edge should be anti-aliased");
    check_true(stroke_outside.a == 0, "stroke far outside should stay transparent");

    return g_failures == 0;
}
