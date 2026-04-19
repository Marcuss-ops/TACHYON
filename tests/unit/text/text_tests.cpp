#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"

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

std::filesystem::path fixture_path() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "fixtures" / "fonts" / "minimal_5x7.bdf";
}

} // namespace

bool run_text_tests() {
    using namespace tachyon::text;
    namespace fs = std::filesystem;

    BitmapFont font;
    check_true(font.load_bdf(fixture_path()), "Load minimal BDF font");
    if (!font.is_loaded()) {
        return false;
    }

    check_true(font.ascent() == 7, "Font ascent parsed");
    check_true(font.line_height() == 7, "Font line height parsed");

    TextStyle style;
    style.pixel_size = 14;
    style.fill_color = tachyon::renderer2d::Color::white();

    {
        TextBox box;
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        const TextLayoutResult layout = layout_text(font, "TACHYON", style, box, TextAlignment::Left);
        check_true(layout.scale == 2, "Layout scale derived from pixel size");
        check_true(layout.lines.size() == 1, "Single-line layout stays on one line");
        check_true(layout.width == 84, "Single-line width is predictable");
        check_true(layout.height == 14, "Single-line height is predictable");
        check_true(!layout.glyphs.empty(), "Layout emitted glyphs");
    }

    {
        TextBox box;
        box.width = 42;
        box.height = 64;
        box.multiline = true;

        const TextLayoutResult layout = layout_text(font, "TACHYON TACHYON", style, box, TextAlignment::Center);
        check_true(layout.lines.size() >= 2, "Wrapping creates multiple lines");
        check_true(layout.glyphs.front().x > 0, "Center alignment offsets the first glyph");
    }

    {
        TextBox box;
        box.width = 84;
        box.height = 32;
        box.multiline = false;

        const TextRasterSurface surface = rasterize_text_rgba(font, "TACHYON", style, box, TextAlignment::Left);
        check_true(surface.width() == 84, "Raster surface width matches layout");
        check_true(surface.height() == 14, "Raster surface height matches layout");

        std::size_t opaque_pixels = 0;
        for (std::uint32_t y = 0; y < surface.height(); ++y) {
            for (std::uint32_t x = 0; x < surface.width(); ++x) {
                if (surface.get_pixel(x, y).a > 0) {
                    ++opaque_pixels;
                }
            }
        }

        check_true(opaque_pixels > 0, "Raster surface contains visible text");
        check_true(surface.get_pixel(0, 0).a == 0, "Background stays transparent");

        fs::create_directories("tests/output");
        check_true(surface.save_png("tests/output/04_text_tachyon.png"), "Save text PNG");
    }

    return g_failures == 0;
}
