#include "tachyon/color/blend_kernel.h"
#include "tachyon/color/blending.h"
#include <cmath>
#include <cassert>
#include <vector>
#include <iostream>

static bool approx_eq(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) < eps;
}

static bool color_approx_eq(tachyon::color::LinearRGBA a, tachyon::color::LinearRGBA b) {
    return approx_eq(a.r, b.r) && approx_eq(a.g, b.g) && approx_eq(a.b, b.b) && approx_eq(a.a, b.a);
}

void test_additive_kernel(size_t count) {
    using namespace tachyon::color;
    
    std::vector<LinearRGBA> src(count, {0.1f, 0.2f, 0.3f, 0.4f});
    std::vector<LinearRGBA> dst(count, {0.5f, 0.5f, 0.5f, 0.5f});
    std::vector<LinearRGBA> expected(count);
    
    for (size_t i = 0; i < count; ++i) {
        expected[i] = blend_additive(src[i], dst[i]);
    }
    
    kernel::additive(src.data(), dst.data(), count);
    
    for (size_t i = 0; i < count; ++i) {
        if (!color_approx_eq(dst[i], expected[i])) {
            std::cerr << "Additive Kernel mismatch at index " << i << " for count " << count << std::endl;
            assert(false);
        }
    }
}

void test_normal_kernel(size_t count) {
    using namespace tachyon::color;
    
    std::vector<LinearRGBA> src(count, {0.8f, 0.1f, 0.1f, 0.5f});
    std::vector<LinearRGBA> dst(count, {0.1f, 0.8f, 0.1f, 1.0f});
    std::vector<LinearRGBA> expected(count);
    
    for (size_t i = 0; i < count; ++i) {
        expected[i] = blend_normal(src[i], dst[i]);
    }
    
    kernel::normal(src.data(), dst.data(), count);
    
    for (size_t i = 0; i < count; ++i) {
        assert(color_approx_eq(dst[i], expected[i]));
    }
}

int main() {
    std::cout << "Running Blend Kernel Tests..." << std::endl;
    
    // Test different counts to check SIMD alignment and fallback logic
    std::vector<size_t> counts = {1, 2, 3, 7, 8, 9, 15, 16, 17};
    
    for (size_t c : counts) {
        test_additive_kernel(c);
        test_normal_kernel(c);
    }
    
    // Test edge cases
    test_additive_kernel(0);
    
    std::cout << "All Blend Kernel Tests PASSED!" << std::endl;
    return 0;
}
