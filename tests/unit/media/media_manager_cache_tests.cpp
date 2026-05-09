#include "tachyon/media/management/media_asset.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
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

bool run_media_manager_cache_tests() {
    using namespace tachyon;

    g_failures = 0;

    media::MediaManager manager;

    const std::string asset_id = "clip_a";
    media::MediaSourceDescriptor descriptor;
    descriptor.original_path = (std::filesystem::temp_directory_path() / "tachyon_media_manager_cache_tests" / "clip_a.mp4").string();
    manager.register_asset(std::make_shared<media::MediaAsset>(asset_id, descriptor));

    auto frame = std::make_unique<renderer2d::SurfaceRGBA>(2, 2);
    frame->clear(renderer2d::Color{0.25f, 0.5f, 0.75f, 1.0f});
    manager.store_video_frame(asset_id, 1.25, std::move(frame));

    DiagnosticBag diagnostics;
    const auto* cached = manager.get_video_frame(asset_id, 1.25, &diagnostics);
    check_true(cached != nullptr, "stored frame should be returned from cache");
    if (cached != nullptr) {
        check_true(cached->width() == 2, "cached frame width should match");
        check_true(cached->height() == 2, "cached frame height should match");
        const auto pixel = cached->get_pixel(0, 0);
        check_true(std::abs(pixel.r - 0.25f) < 0.0001f, "cached frame red channel should match");
        check_true(std::abs(pixel.g - 0.5f) < 0.0001f, "cached frame green channel should match");
        check_true(std::abs(pixel.b - 0.75f) < 0.0001f, "cached frame blue channel should match");
        check_true(std::abs(pixel.a - 1.0f) < 0.0001f, "cached frame alpha channel should match");
    }
    check_true(diagnostics.diagnostics.empty(), "cache hit should not emit diagnostics");

    return g_failures == 0;
}
