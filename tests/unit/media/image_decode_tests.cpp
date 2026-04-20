#include "tachyon/media/image_manager.h"
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

const std::filesystem::path& tests_root() {
    static const std::filesystem::path root = TACHYON_TESTS_SOURCE_DIR;
    return root;
}

} // namespace

bool run_image_decode_tests() {
    using namespace tachyon;

    g_failures = 0;

    const std::filesystem::path out_dir = tests_root() / "output" / "image_decode";
    std::filesystem::create_directories(out_dir);

    media::ImageManager image_manager;

    {
        renderer2d::SurfaceRGBA source(2, 2);
        source.set_pixel(0, 0, renderer2d::Color{255, 0, 0, 255});
        source.set_pixel(1, 0, renderer2d::Color{0, 255, 0, 255});
        source.set_pixel(0, 1, renderer2d::Color{0, 0, 255, 255});
        source.set_pixel(1, 1, renderer2d::Color{255, 255, 255, 255});

        const std::filesystem::path png_path = out_dir / "roundtrip.png";
        check_true(source.save_png(png_path), "PNG fixture should be writable");

        DiagnosticBag diagnostics;
        const auto* decoded = image_manager.get_image(png_path, media::AlphaMode::Straight, &diagnostics);
        check_true(decoded != nullptr, "PNG fixture should decode");
        if (decoded != nullptr) {
            check_true(decoded->width() == 2, "PNG fixture width should match");
            check_true(decoded->height() == 2, "PNG fixture height should match");
            check_true(decoded->get_pixel(0, 0).r == 255, "PNG decode should preserve red channel");
            check_true(decoded->get_pixel(1, 0).g == 255, "PNG decode should preserve green channel");
            check_true(decoded->get_pixel(0, 1).b == 255, "PNG decode should preserve blue channel");
        }
        check_true(!diagnostics.has_warnings(), "PNG decode should not emit warnings");
    }

    {
        const std::filesystem::path jpg_path = tests_root() / "fixtures" / "images" / "minimal.jpg";
        check_true(std::filesystem::exists(jpg_path), "JPEG fixture should exist");

        DiagnosticBag diagnostics;
        const auto* decoded = image_manager.get_image(jpg_path, media::AlphaMode::Straight, &diagnostics);
        check_true(decoded != nullptr, "JPEG fixture should decode");
        if (decoded != nullptr) {
            check_true(decoded->width() == 1, "JPEG fixture width should match");
            check_true(decoded->height() == 1, "JPEG fixture height should match");
            const renderer2d::Color pixel = decoded->get_pixel(0, 0);
            check_true(pixel.r > 200, "JPEG decode should preserve a strong red component");
            check_true(pixel.g < 80, "JPEG decode should keep green low");
            check_true(pixel.b < 80, "JPEG decode should keep blue low");
        }
        check_true(!diagnostics.has_warnings(), "JPEG decode should not emit warnings");
    }

    return g_failures == 0;
}
