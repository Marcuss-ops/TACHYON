#include "image_comparison.h"
#include "tachyon/runtime/runtime_facade.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <iostream>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "stb_image.h"
#include "stb_image_write.h"

namespace tachyon::test {

bool verify_golden_frame(const std::string& test_id, const SceneSpec& scene, int frame_number, double tolerance_rms) {
    auto& facade = RuntimeFacade::instance();
    
    auto compiled = facade.compile_scene(scene);
    if (!compiled.ok()) {
        std::cerr << "[Golden] Failed to compile scene for test: " << test_id << "\n";
        return false;
    }

    auto rendered = facade.render_frame(compiled.value.value(), frame_number);
    if (!rendered.ok() || !rendered.value.value().frame) {
        std::cerr << "[Golden] Failed to render frame " << frame_number << " for test: " << test_id << "\n";
        return false;
    }

    const auto& frame = *rendered.value.value().frame;
    int width = static_cast<int>(frame.width());
    int height = static_cast<int>(frame.height());

    // Convert float surface to RGBA8 for comparison and storage
    std::vector<uint8_t> pixels;
    frame.to_rgba8(pixels);

    std::string tests_src_dir = TACHYON_TESTS_SOURCE_DIR;
    std::string baseline_dir = tests_src_dir + "/golden/baselines/";
    std::string baseline_path = baseline_dir + test_id + ".png";

    const char* update_env = std::getenv("TACHYON_UPDATE_GOLDEN");
    if (update_env && std::string(update_env) == "1") {
        std::cout << "[Golden] Updating baseline: " << baseline_path << "\n";
        if (!save_image(baseline_path, pixels.data(), width, height)) {
            std::cerr << "[Golden] Failed to save baseline image.\n";
            return false;
        }
        return true;
    }

    auto result = compare_to_golden(baseline_path, pixels.data(), width, height, tolerance_rms);
    if (!result.pass) {
        std::cerr << "[Golden] Test '" << test_id << "' failed: " << result.error_message << "\n";
        
        // Save failed image for debugging
        std::string fail_path = tests_src_dir + "/golden/diffs/" + test_id + "_failed.png";
        save_image(fail_path, pixels.data(), width, height);
        std::cerr << "[Golden] Failed image saved to: " << fail_path << "\n";
        return false;
    }

    std::cout << "[Golden] Test '" << test_id << "' passed (RMS=" << result.rms_error << ")\n";
    return true;
}

ComparisonResult compare_to_golden(
    const std::string& baseline_path,
    const uint8_t* candidate_data,
    int width,
    int height,
    double tolerance_rms) {
    
    ComparisonResult result;

    if (!std::filesystem::exists(baseline_path)) {
        result.error_message = "Baseline missing: " + baseline_path;
        return result;
    }

    int b_w, b_h, b_c;
    // stbi_load implementation is in backends
    uint8_t* baseline_data = stbi_load(baseline_path.c_str(), &b_w, &b_h, &b_c, 4);
    if (!baseline_data) {
        result.error_message = "Failed to load baseline: " + baseline_path;
        return result;
    }

    if (b_w != width || b_h != height) {
        stbi_image_free(baseline_data);
        result.error_message = "Dimensions mismatch: golden=" + std::to_string(b_w) + "x" + std::to_string(b_h) + 
                               ", current=" + std::to_string(width) + "x" + std::to_string(height);
        return result;
    }

    double sum_sq_diff = 0;
    double max_diff = 0;
    const size_t num_pixels = static_cast<size_t>(width) * height;

    for (size_t i = 0; i < num_pixels * 4; ++i) {
        double diff = std::abs(static_cast<double>(baseline_data[i]) - static_cast<double>(candidate_data[i])) / 255.0;
        sum_sq_diff += diff * diff;
        max_diff = std::max(max_diff, diff);
    }

    stbi_image_free(baseline_data);

    result.rms_error = std::sqrt(sum_sq_diff / (num_pixels * 4));
    result.max_error = max_diff;
    result.pass = (result.rms_error <= tolerance_rms);

    if (!result.pass) {
        result.error_message = "Visual mismatch! RMS=" + std::to_string(result.rms_error) + 
                               " (tol=" + std::to_string(tolerance_rms) + "), Max=" + std::to_string(result.max_error);
    }

    return result;
}

bool save_image(const std::string& path, const uint8_t* data, int width, int height) {
    // Use project wrapper to avoid STB definition conflicts
    renderer2d::FramebufferRGBA8 fb(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    std::memcpy(fb.mutable_pixels().data(), data, static_cast<size_t>(width) * height * 4);
    return fb.save_png(path);
}

} // namespace tachyon::test
