#include "tachyon/color/blend_kernel.h"
#include "tachyon/color/blending.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

namespace {

static int g_failures = 0;

void check_pixel(const tachyon::color::LinearRGBA& actual, const tachyon::color::LinearRGBA& expected, const std::string& msg) {
    const float eps = 0.0001f;
    bool match = std::abs(actual.r - expected.r) < eps &&
                 std::abs(actual.g - expected.g) < eps &&
                 std::abs(actual.b - expected.b) < eps &&
                 std::abs(actual.a - expected.a) < eps;
    
    if (!match) {
        ++g_failures;
        std::cerr << "FAIL: " << msg << "\n"
                  << "  Expected: {" << expected.r << ", " << expected.g << ", " << expected.b << ", " << expected.a << "}\n"
                  << "  Actual:   {" << actual.r << ", " << actual.g << ", " << actual.b << ", " << actual.a << "}\n";
    }
}

} // namespace

bool run_blend_kernel_tests() {
    using namespace tachyon::color;
    
    std::cout << "Running Blend Kernel tests..." << std::endl;

    const std::size_t count = 16;
    std::vector<LinearRGBA> src(count);
    std::vector<LinearRGBA> dst_simd(count);
    std::vector<LinearRGBA> dst_scalar(count);

    // Initialize with test data
    for (std::size_t i = 0; i < count; ++i) {
        src[i] = {0.1f * i, 0.05f * i, 0.02f * i, 0.5f};
        dst_simd[i] = {0.2f, 0.3f, 0.4f, 0.6f};
        dst_scalar[i] = dst_simd[i];
    }

    // 1. Normal Blend
    kernel::normal(src.data(), dst_simd.data(), count);
    for (std::size_t i = 0; i < count; ++i) {
        dst_scalar[i] = blend_normal(src[i], dst_scalar[i]);
        check_pixel(dst_simd[i], dst_scalar[i], "Normal blend mismatch at index " + std::to_string(i));
    }

    // 2. Additive Blend
    for (std::size_t i = 0; i < count; ++i) {
        dst_simd[i] = {0.5f, 0.5f, 0.5f, 0.5f};
        dst_scalar[i] = dst_simd[i];
    }
    kernel::additive(src.data(), dst_simd.data(), count);
    for (std::size_t i = 0; i < count; ++i) {
        dst_scalar[i] = blend_additive(src[i], dst_scalar[i]);
        check_pixel(dst_simd[i], dst_scalar[i], "Additive blend mismatch at index " + std::to_string(i));
    }

    // 3. Multiply Blend
    for (std::size_t i = 0; i < count; ++i) {
        dst_simd[i] = {0.8f, 0.8f, 0.8f, 0.8f};
        dst_scalar[i] = dst_simd[i];
    }
    kernel::multiply(src.data(), dst_simd.data(), count);
    for (std::size_t i = 0; i < count; ++i) {
        dst_scalar[i] = blend_multiply(src[i], dst_scalar[i]);
        check_pixel(dst_simd[i], dst_scalar[i], "Multiply blend mismatch at index " + std::to_string(i));
    }

    // 4. Screen Blend
    for (std::size_t i = 0; i < count; ++i) {
        dst_simd[i] = {0.5f, 0.5f, 0.5f, 0.5f};
        dst_scalar[i] = dst_simd[i];
    }
    kernel::screen(src.data(), dst_simd.data(), count);
    for (std::size_t i = 0; i < count; ++i) {
        dst_scalar[i] = blend_screen(src[i], dst_scalar[i]);
        check_pixel(dst_simd[i], dst_scalar[i], "Screen blend mismatch at index " + std::to_string(i));
    }

    if (g_failures == 0) {
        std::cout << "All Blend Kernel tests passed!" << std::endl;
    } else {
        std::cout << "Blend Kernel tests failed with " << g_failures << " errors." << std::endl;
    }

    return g_failures == 0;
}
