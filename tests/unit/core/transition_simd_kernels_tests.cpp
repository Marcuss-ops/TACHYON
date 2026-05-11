#include "tachyon/core/transition/transition_simd_kernels.h"
#include <cmath>
#include <cstddef>
#include <vector>
#include <iostream>

namespace {

bool expect_near_vectors(const std::vector<float>& actual, const std::vector<float>& expected) {
    if (actual.size() != expected.size()) {
        std::cerr << "Size mismatch!" << std::endl;
        return false;
    }

    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (std::abs(actual[i] - expected[i]) > 1e-5f) {
            std::cerr << "Mismatch at index " << i << ": expected " << expected[i] << " got " << actual[i] << std::endl;
            return false;
        }
    }
    return true;
}

std::vector<float> make_values(std::size_t count, float scale) {
    std::vector<float> values(count);
    for (std::size_t i = 0; i < count; ++i) {
        values[i] = static_cast<float>(i) * scale - 3.0f;
    }
    return values;
}

bool test_best_matches_scalar() {
    const std::vector<std::size_t> sizes = {
        0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 127, 128, 129
    };

    for (const std::size_t count : sizes) {
        const auto a = make_values(count, 0.25f);
        const auto b = make_values(count, -0.5f);

        std::vector<float> scalar(count);
        std::vector<float> best(count);

        tachyon::runtime::simd::lerp_pixels_scalar(scalar.data(), a.data(), b.data(), count, 0.35f);
        tachyon::runtime::simd::lerp_pixels_best(best.data(), a.data(), b.data(), count, 0.35f);

        if (!expect_near_vectors(best, scalar)) return false;
    }
    return true;
}

bool test_best_boundary() {
    constexpr std::size_t count = 19;
    const auto a = make_values(count, 1.0f);
    const auto b = make_values(count, 2.0f);

    for (const float t : {0.0f, 0.5f, 1.0f}) {
        std::vector<float> scalar(count);
        std::vector<float> best(count);

        tachyon::runtime::simd::lerp_pixels_scalar(scalar.data(), a.data(), b.data(), count, t);
        tachyon::runtime::simd::lerp_pixels_best(best.data(), a.data(), b.data(), count, t);

        if (!expect_near_vectors(best, scalar)) return false;
    }
    return true;
}

#if defined(TACHYON_ENABLE_HIGHWAY)
bool test_highway_matches_scalar() {
    constexpr std::size_t count = 37;
    const auto a = make_values(count, 0.125f);
    const auto b = make_values(count, 0.75f);

    std::vector<float> scalar(count);
    std::vector<float> highway(count);

    tachyon::runtime::simd::lerp_pixels_scalar(scalar.data(), a.data(), b.data(), count, 0.6f);
    tachyon::runtime::simd::lerp_pixels_highway(highway.data(), a.data(), b.data(), count, 0.6f);

    return expect_near_vectors(highway, scalar);
}
#endif

} // namespace

bool run_transition_simd_kernels_tests() {
    if (!test_best_matches_scalar()) return false;
    if (!test_best_boundary()) return false;
#if defined(TACHYON_ENABLE_HIGHWAY)
    if (!test_highway_matches_scalar()) return false;
#endif
    return true;
}