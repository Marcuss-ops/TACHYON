#include "tachyon/text/fonts/core/font.h"

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

bool run_glyph_cache_tests() {
    using namespace tachyon::text;

    BitmapFont font;
    check_true(font.load_bdf(fixture_path()), "Load minimal BDF font");
    if (!font.is_loaded()) {
        return false;
    }

    const GlyphBitmap* base = font.find_glyph(static_cast<std::uint32_t>('T'));
    check_true(base != nullptr, "Base glyph exists");
    if (base == nullptr) {
        return false;
    }

    const GlyphBitmap* scaled_first = font.find_scaled_glyph(static_cast<std::uint32_t>('T'), 2U);
    const GlyphBitmap* scaled_second = font.find_scaled_glyph(static_cast<std::uint32_t>('T'), 2U);
    check_true(scaled_first != nullptr, "Scaled glyph exists");
    check_true(scaled_first == scaled_second, "Scaled glyph is cached");

    if (scaled_first != nullptr) {
        check_true(scaled_first->width == base->width * 2U, "Scaled glyph width doubles");
        check_true(scaled_first->height == base->height * 2U, "Scaled glyph height doubles");
        check_true(scaled_first->alpha_mask.size() == base->alpha_mask.size() * 4U, "Scaled glyph bitmap expands");
    }

    check_true(font.find_scaled_glyph(static_cast<std::uint32_t>('T'), 1U) == base, "Scale 1 reuses base glyph");

    return g_failures == 0;
}
