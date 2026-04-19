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
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        TextLayoutOptions tracking;
        tracking.tracking = 1.0f;

        const TextLayoutResult base_layout = layout_text(font, "TA", style, box, TextAlignment::Left);
        const TextLayoutResult tracked_layout = layout_text(font, "TA", style, box, TextAlignment::Left, tracking);
        check_true(tracked_layout.width > base_layout.width, "Tracking increases laid-out width");
    }

    {
        TextBox box;
        box.width = 42;
        box.height = 64;
        box.multiline = true;

        const TextLayoutResult layout = layout_text(font, "TACHYON TACHYON", style, box, TextAlignment::Center);
        check_true(layout.lines.size() >= 2, "Wrapping creates multiple lines");
        check_true(layout.glyphs.front().x > 0, "Center alignment offsets the first glyph");
        if (!layout.lines.empty() && layout.lines.front().glyph_count > 0) {
            check_true(!layout.glyphs[layout.lines.front().glyph_count - 1].whitespace, "Wrapping should prefer a word boundary");
        }
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
        std::size_t transparent_pixels = 0;
        for (std::uint32_t y = 0; y < surface.height(); ++y) {
            for (std::uint32_t x = 0; x < surface.width(); ++x) {
                if (surface.get_pixel(x, y).a > 0) {
                    ++opaque_pixels;
                } else {
                    ++transparent_pixels;
                }
            }
        }

        check_true(opaque_pixels > 0, "Raster surface contains visible text");
        check_true(transparent_pixels > 0, "Background stays transparent");

        fs::create_directories("tests/output");
        check_true(surface.save_png("tests/output/04_text_tachyon.png"), "Save text PNG");
    }

    {
        TextBox box;
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        TextLayoutOptions layout_options;
        layout_options.tracking = 1.0f;
        const TextLayoutResult layout = layout_text(font, "TA", style, box, TextAlignment::Left, layout_options);
        const TextRasterSurface base_surface = rasterize_text_rgba(font, "TA", style, box, TextAlignment::Left, layout_options);

        TextAnimationOptions animation;
        animation.enabled = true;
        animation.per_glyph_opacity_drop = 1.0f;

        const TextRasterSurface animated_surface = rasterize_text_rgba(font, "TA", style, box, TextAlignment::Left, layout_options, animation);

        std::size_t base_opaque = 0;
        std::size_t animated_opaque = 0;
        for (std::uint32_t y = 0; y < base_surface.height(); ++y) {
            for (std::uint32_t x = 0; x < base_surface.width(); ++x) {
                if (base_surface.get_pixel(x, y).a > 0) {
                    ++base_opaque;
                }
                if (animated_surface.get_pixel(x, y).a > 0) {
                    ++animated_opaque;
                }
            }
        }

        check_true(animated_opaque < base_opaque, "Per-glyph opacity animation reduces visible pixels");
        if (layout.glyphs.size() >= 2) {
            const auto& second = layout.glyphs[1];
            const std::uint32_t sample_x = static_cast<std::uint32_t>(std::max<std::int32_t>(0, second.x + std::max<std::int32_t>(1, second.width / 2)));
            const std::uint32_t sample_y = static_cast<std::uint32_t>(std::max<std::int32_t>(0, second.y + std::max<std::int32_t>(1, second.height / 2)));
            check_true(animated_surface.get_pixel(sample_x, sample_y).a == 0, "Per-glyph opacity can hide later glyphs");
        }
    }

    return g_failures == 0;
}
