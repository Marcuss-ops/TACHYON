#include "tachyon/media/image_manager.h"

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

bool run_image_manager_tests() {
    using namespace tachyon;

    g_failures = 0;

    media::ImageManager image_manager;
    image_manager.clear_cache();

    DiagnosticBag diagnostics;
    const auto* image = image_manager.get_image(
        std::filesystem::path{"__missing_image_for_test__.png"},
        media::AlphaMode::Straight,
        &diagnostics);
    check_true(image != nullptr, "missing image should return a fallback surface");
    if (image != nullptr) {
        check_true(image->width() == 256, "fallback surface should use the expected width");
        check_true(image->height() == 256, "fallback surface should use the expected height");
    }
    check_true(diagnostics.has_warnings(), "missing image should emit a warning diagnostic");
    check_true(!diagnostics.diagnostics.empty(), "missing image should record at least one diagnostic");
    if (!diagnostics.diagnostics.empty()) {
        check_true(diagnostics.diagnostics.front().code == "media.image.decode_failed", "diagnostic code should identify the decode failure");
    }

    const DiagnosticBag consumed = image_manager.consume_diagnostics();
    check_true(!consumed.diagnostics.empty(), "consumed diagnostics should include the recorded warning");
    const DiagnosticBag consumed_again = image_manager.consume_diagnostics();
    check_true(consumed_again.diagnostics.empty(), "consume_diagnostics should clear buffered warnings");

    return g_failures == 0;
}
