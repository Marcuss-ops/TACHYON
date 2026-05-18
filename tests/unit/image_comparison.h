#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon::test {

struct ComparisonResult {
    double rms_error{0.0};
    double max_error{0.0};
    bool pass{false};
    std::string error_message;
};

/**
 * @brief High-level helper to render a scene frame and verify it against a golden baseline.
 * Uses TACHYON_UPDATE_GOLDEN=1 env var to update baselines.
 */
bool verify_golden_frame(const std::string& test_id, const SceneSpec& scene, int frame_number, double tolerance_rms = 0.01);

/**
 * @brief Helper to render a JIT scene over a sequence of strategic frames and verify them against golden baselines.
 */
bool verify_golden_sequence(
    const std::string& test_id,
    const SceneSpec& scene,
    const std::vector<int>& frames,
    double tolerance_rms = 0.01
);

/**
 * @brief Compares two images and returns detailed error metrics.
 * 
 * @param baseline_path Path to the reference "golden" image.
 * @param candidate_data Raw RGBA8 pixel data of the current render.
 * @param width Image width.
 * @param height Image height.
 * @param tolerance_rms Maximum allowed RMS error (default 0.01).
 * @return ComparisonResult 
 */
ComparisonResult compare_to_golden(
    const std::string& baseline_path,
    const uint8_t* candidate_data,
    int width,
    int height,
    double tolerance_rms = 0.02
);

/**
 * @brief Saves an image to disk for manual inspection.
 */
bool save_image(const std::string& path, const uint8_t* data, int width, int height);

} // namespace tachyon::test
