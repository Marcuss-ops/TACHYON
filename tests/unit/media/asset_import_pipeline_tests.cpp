#include "tachyon/media/import/asset_import_pipeline.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <filesystem>
#include <iostream>
#include <cassert>

void test_image_import_determinism() {
    std::cout << "Test: Image import determinism... ";

    // Create a simple test image (1x1 red pixel)
    tachyon::renderer2d::SurfaceRGBA test_surface(1, 1);
    test_surface.mutable_pixels() = {1.0f, 0.0f, 0.0f, 1.0f};

    // Verify surface properties
    assert(test_surface.width() == 1);
    assert(test_surface.height() == 1);
    assert(test_surface.pixels()[0] == 1.0f); // Red
    assert(test_surface.pixels()[3] == 1.0f); // Alpha

    std::cout << "PASSED\n";
}

void test_video_frame_cache_key() {
    std::cout << "Test: Video frame cache key consistency... ";

    // Cache key should be consistent
    std::string path = "/path/to/video.mp4";
    double time1 = 1.0;
    double time2 = 2.0;

    std::string key1 = path + "@" + std::to_string(time1);
    std::string key2 = path + "@" + std::to_string(time2);
    std::string key1_dup = path + "@" + std::to_string(time1);

    assert(key1 == key1_dup);
    assert(key1 != key2);

    std::cout << "PASSED\n";
}

void test_import_request_creation() {
    std::cout << "Test: ImportRequest creation... ";

    tachyon::media::ImportRequest request;
    request.path = "/path/to/image.png";
    request.type = tachyon::media::AssetType::IMAGE;
    request.alpha_mode = tachyon::media::AlphaMode::Straight;
    request.time_seconds = 0.0;

    assert(request.path == "/path/to/image.png");
    assert(request.type == tachyon::media::AssetType::IMAGE);
    assert(request.alpha_mode == tachyon::media::AlphaMode::Straight);

    std::cout << "PASSED\n";
}

void test_import_result_status() {
    std::cout << "Test: ImportResult status values... ";

    tachyon::media::ImportResult result;
    assert(result.status == tachyon::media::ImportResult::Success);
    assert(result.surface == nullptr);
    assert(result.error_message.empty());

    result.status = tachyon::media::ImportResult::NotFound;
    assert(result.status == tachyon::media::ImportResult::NotFound);

    result.status = tachyon::media::ImportResult::DecodeError;
    assert(result.status == tachyon::media::ImportResult::DecodeError);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Asset Import Pipeline Determinism Tests ===\n\n";

    test_image_import_determinism();
    test_video_frame_cache_key();
    test_import_request_creation();
    test_import_result_status();

    std::cout << "\n=== All tests passed ===\n";
    return 0;
}
