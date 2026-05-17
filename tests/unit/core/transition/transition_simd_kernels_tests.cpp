#include "test_utils.h"
#include "tachyon/core/transition/transition_simd_kernels.h"
#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <random>

namespace tachyon::runtime::simd {

bool run_simd_blend_tests() {
    std::cout << "[SIMD] Starting transition SIMD kernels test...\n";

    // Prepare float buffers for 1080p and 4K
    // 1080p: 1920 * 1080 * 4 floats
    // 4K: 3840 * 2160 * 4 floats
    const std::size_t count_1080p = 1920 * 1080 * 4;
    const std::size_t count_4k = 3840 * 2160 * 4;

    std::vector<float> a_1080p(count_1080p);
    std::vector<float> b_1080p(count_1080p);
    std::vector<float> out_scalar_1080p(count_1080p);
    std::vector<float> out_simd_1080p(count_1080p);

    // Initialize with random values
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (std::size_t i = 0; i < count_1080p; ++i) {
        a_1080p[i] = dist(rng);
        b_1080p[i] = dist(rng);
    }

    const float blend = 0.35f;

    // 1. Correctness check for 1080p
    std::cout << "[SIMD] Running 1080p scalar and SIMD/best blend...\n";
    lerp_pixels_scalar(out_scalar_1080p.data(), a_1080p.data(), b_1080p.data(), count_1080p, blend);
    lerp_pixels_best(out_simd_1080p.data(), a_1080p.data(), b_1080p.data(), count_1080p, blend);

    // Verify equality
    for (std::size_t i = 0; i < count_1080p; ++i) {
        float diff = std::abs(out_scalar_1080p[i] - out_simd_1080p[i]);
        if (diff > 1e-5f) {
            std::cerr << "[SIMD] ERROR: Mismatch at index " << i 
                      << "! Scalar: " << out_scalar_1080p[i] 
                      << ", SIMD: " << out_simd_1080p[i] << "\n";
            return false;
        }
    }
    std::cout << "[SIMD] Correctness test passed for 1080p!\n";

    // 2. Benchmark 1080p
    auto start_scalar = std::chrono::high_resolution_clock::now();
    for (int run = 0; run < 10; ++run) {
        lerp_pixels_scalar(out_scalar_1080p.data(), a_1080p.data(), b_1080p.data(), count_1080p, blend);
    }
    auto end_scalar = std::chrono::high_resolution_clock::now();
    double ms_scalar_1080p = std::chrono::duration<double, std::milli>(end_scalar - start_scalar).count() / 10.0;

    auto start_simd = std::chrono::high_resolution_clock::now();
    for (int run = 0; run < 10; ++run) {
        lerp_pixels_best(out_simd_1080p.data(), a_1080p.data(), b_1080p.data(), count_1080p, blend);
    }
    auto end_simd = std::chrono::high_resolution_clock::now();
    double ms_simd_1080p = std::chrono::duration<double, std::milli>(end_simd - start_simd).count() / 10.0;

    std::cout << "[SIMD] 1080p Benchmark (average of 10 runs):\n";
    std::cout << "       Scalar: " << ms_scalar_1080p << " ms\n";
    std::cout << "       SIMD/Best: " << ms_simd_1080p << " ms\n";
    if (ms_simd_1080p > 0) {
        std::cout << "       Speedup: " << (ms_scalar_1080p / ms_simd_1080p) << "x\n";
    }

    // 3. Correctness & Benchmark 4K
    std::cout << "[SIMD] Preparing 4K buffers...\n";
    std::vector<float> a_4k(count_4k);
    std::vector<float> b_4k(count_4k);
    std::vector<float> out_scalar_4k(count_4k);
    std::vector<float> out_simd_4k(count_4k);

    for (std::size_t i = 0; i < count_4k; ++i) {
        a_4k[i] = dist(rng);
        b_4k[i] = dist(rng);
    }

    std::cout << "[SIMD] Running 4K scalar and SIMD/best blend...\n";
    lerp_pixels_scalar(out_scalar_4k.data(), a_4k.data(), b_4k.data(), count_4k, blend);
    lerp_pixels_best(out_simd_4k.data(), a_4k.data(), b_4k.data(), count_4k, blend);

    for (std::size_t i = 0; i < count_4k; ++i) {
        float diff = std::abs(out_scalar_4k[i] - out_simd_4k[i]);
        if (diff > 1e-5f) {
            std::cerr << "[SIMD] ERROR: 4K Mismatch at index " << i 
                      << "! Scalar: " << out_scalar_4k[i] 
                      << ", SIMD: " << out_simd_4k[i] << "\n";
            return false;
        }
    }
    std::cout << "[SIMD] Correctness test passed for 4K!\n";

    start_scalar = std::chrono::high_resolution_clock::now();
    for (int run = 0; run < 5; ++run) {
        lerp_pixels_scalar(out_scalar_4k.data(), a_4k.data(), b_4k.data(), count_4k, blend);
    }
    end_scalar = std::chrono::high_resolution_clock::now();
    double ms_scalar_4k = std::chrono::duration<double, std::milli>(end_scalar - start_scalar).count() / 5.0;

    start_simd = std::chrono::high_resolution_clock::now();
    for (int run = 0; run < 5; ++run) {
        lerp_pixels_best(out_simd_4k.data(), a_4k.data(), b_4k.data(), count_4k, blend);
    }
    end_simd = std::chrono::high_resolution_clock::now();
    double ms_simd_4k = std::chrono::duration<double, std::milli>(end_simd - start_simd).count() / 5.0;

    std::cout << "[SIMD] 4K Benchmark (average of 5 runs):\n";
    std::cout << "       Scalar: " << ms_scalar_4k << " ms\n";
    std::cout << "       SIMD/Best: " << ms_simd_4k << " ms\n";
    if (ms_simd_4k > 0) {
        std::cout << "       Speedup: " << (ms_scalar_4k / ms_simd_4k) << "x\n";
    }

    return true;
}

} // namespace tachyon::runtime::simd
