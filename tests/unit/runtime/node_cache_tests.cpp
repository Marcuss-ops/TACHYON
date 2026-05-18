#include "test_utils.h"
#include "tachyon/runtime/cache/node_cache.h"
#include "tachyon/runtime/cache/sharded_lru_cache.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <iostream>
#include <memory>
#include <string>
#include <cassert>
#include <thread>
#include <vector>

bool run_node_cache_tests() {
    std::cout << "[NodeCacheTests] Starting NodeCache unit tests...\n";
    using namespace tachyon;
    using namespace tachyon::renderer2d;

    // 1. Test Key Equality and Hashing
    {
        NodeCacheKey key1{123, 456, 789, 1920, 1080, 0.0, "sRGB", "high"};
        NodeCacheKey key2{123, 456, 789, 1920, 1080, 0.0, "sRGB", "high"};
        NodeCacheKey key3{123, 456, 789, 1920, 1080, 0.0, "DisplayP3", "high"};
        NodeCacheKey key4{123, 999, 789, 1920, 1080, 0.0, "sRGB", "high"};

        if (!(key1 == key2)) {
            std::cerr << "[NodeCacheTests] FAIL: Key equality operator failed.\n";
            return false;
        }

        if (key1 == key3 || key1 == key4) {
            std::cerr << "[NodeCacheTests] FAIL: Key inequality operator failed.\n";
            return false;
        }

        std::hash<NodeCacheKey> hasher;
        if (hasher(key1) != hasher(key2)) {
            std::cerr << "[NodeCacheTests] FAIL: Hasher is not deterministic.\n";
            return false;
        }
        std::cout << "[NodeCacheTests] Key equality and hashing verified!\n";
    }

    // 2. Test Store & Lookup
    {
        NodeCache cache;
        cache.set_capacity_bytes(1024 * 1024 * 10); // 10MB budget
        NodeCacheKey key{123, 456, 0, 100, 100, 0.0, "sRGB", "high"};

        auto initial_lookup = cache.lookup(key);
        if (initial_lookup != nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Lookup should be null for un-cached item.\n";
            return false;
        }

        auto surface = std::make_shared<SurfaceRGBA>(100, 100);
        surface->clear(Color::red());

        cache.store(key, surface);

        auto found = cache.lookup(key);
        if (found == nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Lookup failed after store.\n";
            return false;
        }

        if (found->width() != 100 || found->height() != 100) {
            std::cerr << "[NodeCacheTests] FAIL: Retrieved surface dimensions incorrect.\n";
            return false;
        }

        // Compare color
        Color c = found->get_pixel(50, 50);
        if (c.r != 1.0f || c.g != 0.0f || c.b != 0.0f || c.a != 1.0f) {
            std::cerr << "[NodeCacheTests] FAIL: Retrieved pixel values incorrect.\n";
            return false;
        }
        std::cout << "[NodeCacheTests] Store and lookup verified!\n";
    }

    // 3. Test LRU Eviction & Budget constraints
    {
        // 100x100 SurfaceRGBA is 100 * 100 * 4 * sizeof(float) = 160000 bytes (~156KB)
        // Let's set a capacity that fits exactly 2 surfaces, i.e., 350,000 bytes
        NodeCache cache;
        cache.set_capacity_bytes(350000);

        NodeCacheKey key1{123, 1, 0, 100, 100, 0.0, "sRGB", "high"};
        NodeCacheKey key2{123, 2, 0, 100, 100, 0.0, "sRGB", "high"};
        NodeCacheKey key3{123, 3, 0, 100, 100, 0.0, "sRGB", "high"};

        auto s1 = std::make_shared<SurfaceRGBA>(100, 100);
        auto s2 = std::make_shared<SurfaceRGBA>(100, 100);
        auto s3 = std::make_shared<SurfaceRGBA>(100, 100);

        cache.store(key1, s1);
        cache.store(key2, s2);

        if (cache.lookup(key2) == nullptr || cache.lookup(key1) == nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Pre-eviction lookup failed.\n";
            return false;
        }

        // Store key3. Since we fit 2 and key3 pushes it over capacity, the least recently used one (key2, because we just looked up key1) should be evicted!
        cache.store(key3, s3);

        if (cache.lookup(key1) == nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Most recently used key1 was incorrectly evicted.\n";
            return false;
        }

        if (cache.lookup(key3) == nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Newly inserted key3 was evicted.\n";
            return false;
        }

        if (cache.lookup(key2) != nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Least recently used key2 was not evicted.\n";
            return false;
        }

        std::cout << "[NodeCacheTests] LRU Eviction and capacity limit verified!\n";
    }

    // 4. Test Cache Clear
    {
        NodeCache cache;
        cache.set_capacity_bytes(1024 * 1024);
        NodeCacheKey key{1, 2, 3, 10, 10, 0.0, "sRGB", "high"};
        cache.store(key, std::make_shared<SurfaceRGBA>(10, 10));

        if (cache.current_usage_bytes() == 0) {
            std::cerr << "[NodeCacheTests] FAIL: Usage bytes should be non-zero after store.\n";
            return false;
        }

        cache.clear();

        if (cache.current_usage_bytes() != 0) {
            std::cerr << "[NodeCacheTests] FAIL: Usage bytes should be zero after clear.\n";
            return false;
        }

        if (cache.lookup(key) != nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Lookup should be null after clear.\n";
            return false;
        }
        std::cout << "[NodeCacheTests] Cache clear verified!\n";
    }

    // 5. Test NodeCache Move Semantics and Iterator Correction
    {
        NodeCache cache;
        cache.set_capacity_bytes(1024 * 1024 * 10);
        NodeCacheKey key1{123, 1, 0, 10, 10, 0.0, "sRGB", "high"};
        NodeCacheKey key2{123, 2, 0, 10, 10, 0.0, "sRGB", "high"};

        cache.store(key1, std::make_shared<SurfaceRGBA>(10, 10));
        cache.store(key2, std::make_shared<SurfaceRGBA>(10, 10));

        NodeCache moved_cache = std::move(cache);

        if (moved_cache.lookup(key1) == nullptr || moved_cache.lookup(key2) == nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: Move constructor failed to preserve cached items.\n";
            return false;
        }

        // Verify that lookup touching works perfectly inside the moved cache (validating iterator correction)
        moved_cache.lookup(key1);

        // Lower capacity to trigger eviction of key2
        moved_cache.set_capacity_bytes(static_cast<std::size_t>(10 * 10 * 4 * sizeof(float) * 1.5)); // Fits only 1 surface

        if (moved_cache.lookup(key1) == nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: MRU key1 incorrect eviction after move and capacity adjustment.\n";
            return false;
        }
        if (moved_cache.lookup(key2) != nullptr) {
            std::cerr << "[NodeCacheTests] FAIL: LRU key2 was not evicted after move and capacity adjustment.\n";
            return false;
        }

        std::cout << "[NodeCacheTests] NodeCache move semantics and iterator correction verified!\n";
    }

    // 6. Test ShardedLruCache Concurrency and Sharding Safety
    {
        ShardedLruCache<std::uint64_t, double> sharded_cache(1024 * 1024 * 10); // 10MB
        constexpr int num_threads = 8;
        constexpr int ops_per_thread = 500;
        std::vector<std::thread> thread_pool;

        for (int t = 0; t < num_threads; ++t) {
            thread_pool.emplace_back([&sharded_cache, t]() {
                for (int i = 0; i < ops_per_thread; ++i) {
                    std::uint64_t key = t * 10000 + i;
                    sharded_cache.store(key, static_cast<double>(key), sizeof(double));
                    double val = 0.0;
                    if (sharded_cache.lookup(key, val)) {
                        assert(val == static_cast<double>(key));
                    }
                }
            });
        }

        for (auto& thread : thread_pool) {
            thread.join();
        }

        std::cout << "[NodeCacheTests] ShardedLruCache concurrency and sharding safety verified!\n";
    }

    std::cout << "[NodeCacheTests] All NodeCache unit tests passed successfully!\n";
    return true;
}
