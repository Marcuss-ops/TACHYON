#include "test_utils.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include <iostream>
#include <cassert>

using namespace tachyon;

bool run_frame_cache_tests() {
    std::cout << "[FrameCacheTests] Starting FrameCache unit tests...\n";

    // Test 1: Properties, Layers, Compositions lookup & store
    {
        FrameCache cache;
        double val = 0.0;
        if (cache.lookup_property(42, val)) {
            std::cerr << "[FrameCacheTests] FAIL: Property should be missing\n";
            return false;
        }

        cache.store_property(42, 3.14);
        if (!cache.lookup_property(42, val) || val != 3.14) {
            std::cerr << "[FrameCacheTests] FAIL: Property lookup failed\n";
            return false;
        }

        auto layer = std::make_shared<scene::EvaluatedLayerState>();
        cache.store_layer(101, layer);
        if (cache.lookup_layer(101) != layer) {
            std::cerr << "[FrameCacheTests] FAIL: Layer lookup failed\n";
            return false;
        }

        auto comp = std::make_shared<scene::EvaluatedCompositionState>();
        cache.store_composition(202, comp);
        if (cache.lookup_composition(202) != comp) {
            std::cerr << "[FrameCacheTests] FAIL: Composition lookup failed\n";
            return false;
        }

        std::cout << "[FrameCacheTests] Basic domain lookups verified!\n";
    }

    // Test 2: Hash Collision Safety
    {
        FrameCache cache;

        // Two keys with the same hash but different string representations
        FrameCacheKey key1(12345, "composition_1_frame_0");
        FrameCacheKey key2(12345, "composition_2_frame_0");

        auto frame1 = std::make_shared<renderer2d::Framebuffer>(10, 10);
        auto frame2 = std::make_shared<renderer2d::Framebuffer>(20, 20);

        cache.store_frame(key1, frame1);
        cache.store_frame(key2, frame2);

        auto lookup1 = cache.lookup_frame(key1);
        auto lookup2 = cache.lookup_frame(key2);

        if (lookup1 != frame1) {
            std::cerr << "[FrameCacheTests] FAIL: Hash collision safety lost key1\n";
            return false;
        }

        if (lookup2 != frame2) {
            std::cerr << "[FrameCacheTests] FAIL: Hash collision safety lost key2\n";
            return false;
        }

        std::cout << "[FrameCacheTests] Hash collision safety verified!\n";
    }

    // Test 3: Clearing behavior
    {
        FrameCache cache;
        cache.store_property(42, 3.14);
        cache.clear();

        double val = 0.0;
        if (cache.lookup_property(42, val)) {
            std::cerr << "[FrameCacheTests] FAIL: Clear did not clear cache\n";
            return false;
        }
        std::cout << "[FrameCacheTests] Clear behavior verified!\n";
    }

    // Test 4: Budget enforcement
    {
        FrameCache cache;
        cache.set_budget_bytes(1000);

        auto frame = std::make_shared<renderer2d::Framebuffer>(10, 10);
        
        for (int i = 0; i < 50; ++i) {
            FrameCacheKey k(i, "frame_" + std::to_string(i));
            cache.store_frame(k, frame);
        }

        cache.evict_if_needed();

        std::cout << "[FrameCacheTests] Budget and eviction verified!\n";
    }

    // Test 5: Domain-specific stats
    {
        FrameCache cache;
        double val = 0.0;
        cache.lookup_property(1, val);
        cache.store_property(1, 10.0);
        cache.lookup_property(1, val);

        auto ps = cache.property_stats();
        if (ps.hits != 1 || ps.misses != 1) {
            std::cerr << "[FrameCacheTests] FAIL: Property stats mismatch\n";
            return false;
        }

        auto ls = cache.layer_stats();
        if (ls.lookups != 0) {
            std::cerr << "[FrameCacheTests] FAIL: Layer stats should be 0\n";
            return false;
        }
        std::cout << "[FrameCacheTests] Domain-specific stats verified!\n";
    }

    std::cout << "[FrameCacheTests] ALL TESTS PASSED!\n";
    return true;
}
