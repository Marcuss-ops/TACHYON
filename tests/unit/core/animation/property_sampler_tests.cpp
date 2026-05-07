#include "tachyon/core/animation/property_sampler.h"
#include <iostream>
#include <string>
#include <cmath>

using namespace tachyon;
using namespace tachyon::animation;

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& msg) {
    if (!condition) {
        ++g_failures;
        std::cerr << "  FAIL: " << msg << '\n';
    }
}

bool nearly_eq(double a, double b, double eps = 1e-4) {
    return std::abs(a - b) <= eps;
}

bool nearly_eq(float a, float b, float eps = 1e-4f) {
    return std::abs(a - b) <= eps;
}

void test_sample_scalar() {
    std::vector<ScalarKeyframeSpec> empty_kfs;
    check_true(nearly_eq(sample_scalar(10.0, empty_kfs, 1.5, 0.0), 10.0), "Scalar fallback to static");
    check_true(nearly_eq(sample_scalar(std::nullopt, empty_kfs, 1.5, 5.0), 5.0), "Scalar fallback to arg");

    std::vector<ScalarKeyframeSpec> kfs = {
        {0.0, 0.0, EasingPreset::None, InterpolationMode::Linear},
        {1.0, 10.0, EasingPreset::None, InterpolationMode::Linear}
    };
    check_true(nearly_eq(sample_scalar(std::nullopt, kfs, 0.5, 0.0), 5.0), "Scalar interpolate");
    check_true(nearly_eq(sample_scalar(std::nullopt, kfs, 2.0, 0.0), 10.0), "Scalar clamp max");
}

void test_sample_vector2() {
    std::vector<Vector2KeyframeSpec> kfs = {
        {0.0, math::Vector2{0.0f, 0.0f}, math::Vector2::zero(), math::Vector2::zero(), EasingPreset::None, InterpolationMode::Linear},
        {1.0, math::Vector2{10.0f, 20.0f}, math::Vector2::zero(), math::Vector2::zero(), EasingPreset::None, InterpolationMode::Linear}
    };
    math::Vector2 mid = sample_vector2(std::nullopt, kfs, 0.5, math::Vector2::zero());
    check_true(nearly_eq(mid.x, 5.0f) && nearly_eq(mid.y, 10.0f), "Vector2 interpolate");
}

void test_sample_color() {
    std::vector<ColorKeyframeSpec> kfs = {
        {0.0, ColorSpec{0, 0, 0, 0}, EasingPreset::None, InterpolationMode::Linear},
        {1.0, ColorSpec{255, 100, 50, 200}, EasingPreset::None, InterpolationMode::Linear}
    };
    ColorSpec mid = sample_color(std::nullopt, kfs, 0.5, ColorSpec{255, 255, 255, 255});
    check_true(mid.r == 127 && mid.g == 50 && mid.b == 25 && mid.a == 100, "Color interpolate");
}

} // namespace

bool run_property_sampler_tests() {
    g_failures = 0;
    std::cout << "--- Property Sampler Tests ---\n";
    test_sample_scalar();
    test_sample_vector2();
    test_sample_color();
    if (g_failures == 0) {
        std::cout << "  all property sampler tests passed\n";
    }
    return g_failures == 0;
}
