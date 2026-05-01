#include "tachyon/renderer3d/effects/motion_blur.h"
#include <cassert>
#include <string>
#include <vector>

void test_motion_blur_sample_count_1() {
    tachyon::renderer3d::MotionBlurRenderer::MotionBlurConfig config;
    config.enabled = true;
    config.samples = 1;
    tachyon::renderer3d::MotionBlurRenderer renderer(config);
    assert(renderer.get_subframe_count() == 1);
}

void test_motion_blur_weight_curves() {
    tachyon::renderer3d::MotionBlurRenderer::MotionBlurConfig config;
    config.enabled = true;
    config.samples = 2;
    config.weight_curve = tachyon::renderer3d::MotionBlurRenderer::MotionBlurWeightCurve::kBox;
    tachyon::renderer3d::MotionBlurRenderer renderer(config);
    float w = renderer.evaluate_weight(0, 2);
    assert(std::abs(w - 1.0f) < 1e-6f);
}

void test_motion_blur_cache_identity() {
    tachyon::renderer3d::MotionBlurRenderer::MotionBlurConfig config;
    config.enabled = true;
    config.samples = 4;
    config.shutter_angle = 90.0;
    config.weight_curve = tachyon::renderer3d::MotionBlurRenderer::MotionBlurWeightCurve::kTriangle;
    tachyon::renderer3d::MotionBlurRenderer renderer(config);
    std::string id = renderer.cache_identity();
    assert(id.find("mb_samples:4") != std::string::npos);
}

bool run_motion_blur_tests() {
    test_motion_blur_sample_count_1();
    test_motion_blur_weight_curves();
    test_motion_blur_cache_identity();
    return true;
}
