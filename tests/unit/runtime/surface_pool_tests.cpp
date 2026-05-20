#include "test_utils.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include <iostream>
#include <cassert>

using namespace tachyon;

bool run_surface_pool_tests() {
    std::cout << "[SurfacePoolTests] Starting SurfacePool unit tests...\n";

    // Test 1: prewarm allocation and availability counts
    {
        auto pool = std::make_shared<SurfacePool>();
        assert(pool->available_count() == 0);
        assert(pool->current_bytes() == 0);

        pool->prewarm(100, 100, 5);

        assert(pool->available_count() == 5);
        
        // Width * Height * 4 floats/pixel + Width * Height floats depth buffer
        std::size_t expected_bytes_per_surface = (100 * 100 * 4 + 100 * 100) * sizeof(float);
        std::size_t expected_total_bytes = expected_bytes_per_surface * 5;
        assert(pool->current_bytes() == expected_total_bytes);

        (void)expected_total_bytes;
        std::cout << "[SurfacePoolTests] Prewarm allocation and byte metrics verified!\n";
    }

    // Test 2: acquire reuse and count release
    {
        auto pool = std::make_shared<SurfacePool>();
        pool->prewarm(64, 64, 2);

        assert(pool->available_count() == 2);

        {
            auto fb1 = pool->acquire(64, 64);
            assert(pool->available_count() == 1);

            auto fb2 = pool->acquire(64, 64);
            assert(pool->available_count() == 0);
            (void)fb1;
            (void)fb2;
        }

        // After going out of scope, the surfaces must be returned to the pool
        assert(pool->available_count() == 2);

        std::cout << "[SurfacePoolTests] Acquire and automatic release verified!\n";
    }

    // Test 3: clearing the pool
    {
        auto pool = std::make_shared<SurfacePool>();
        pool->prewarm(32, 32, 10);
        assert(pool->available_count() == 10);

        pool->clear();
        assert(pool->available_count() == 0);
        assert(pool->current_bytes() == 0);

        std::cout << "[SurfacePoolTests] Clear behavior verified!\n";
    }

    std::cout << "[SurfacePoolTests] ALL TESTS PASSED!\n";
    return true;
}
