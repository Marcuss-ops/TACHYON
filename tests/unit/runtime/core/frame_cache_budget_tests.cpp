#include "tachyon/runtime/cache/frame_cache.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_frame_cache_budget_tests() {
    using namespace tachyon;

    g_failures = 0;

    // --- Budget enforcement ---
    {
        FrameCache cache;
        cache.set_budget_bytes(100);

        // 10 doubles = 10 * 8 = 80 bytes. Fits in 100.
        for (uint64_t i = 0; i < 10; ++i) {
            cache.store_property(i, static_cast<double>(i));
        }
        check(cache.current_usage_bytes() <= 100, "Usage stays under 100-byte budget after 10 stores");
        check(cache.current_usage_bytes() == 80, "Usage is exactly 80 bytes for 10 doubles");

        // 5 more doubles push to 120. Eviction must kick in.
        for (uint64_t i = 10; i < 15; ++i) {
            cache.store_property(i, static_cast<double>(i));
        }
        check(cache.current_usage_bytes() <= 100, "Usage stays under budget after eviction");
    }

    // --- LRU eviction order ---
    {
        FrameCache cache;
        cache.set_budget_bytes(20); // holds 2 doubles (16 bytes) comfortably

        cache.store_property(1, 1.0);
        cache.store_property(2, 2.0);

        double val = 0.0;
        check(cache.lookup_property(1, val), "Key 1 present after both stores");
        check(cache.lookup_property(2, val), "Key 2 present after both stores");

        // Store 3: 16+8=24 > 20, must evict. LRU is key 1 (lookup(2) was most recent touch,
        // then lookup(1) touched it last — so actually 2 is LRU). Let's be precise:
        // after store(1): lru=[1]
        // after store(2): lru=[2,1]
        // after lookup(1): lru=[1,2]  (1 touched, moved to front)
        // after lookup(2): lru=[2,1]  (2 touched, moved to front)
        // evict_if_needed for store(3): evict back = key 1
        cache.store_property(3, 3.0);
        check(!cache.lookup_property(1, val), "Key 1 evicted as LRU after store(3)");
        check(cache.lookup_property(2, val), "Key 2 still present after store(3)");
        check(cache.lookup_property(3, val), "Key 3 present after store(3)");

        // Touch 2, then store 4: should evict 3 (now LRU)
        cache.lookup_property(2, val); // touch 2 → lru=[2,3]
        cache.store_property(4, 4.0); // evict 3
        check(!cache.lookup_property(3, val), "Key 3 evicted as LRU after store(4)");
        check(cache.lookup_property(2, val), "Key 2 still present after store(4)");
        check(cache.lookup_property(4, val), "Key 4 present after store(4)");
    }

    // --- Budget respected after clear ---
    {
        FrameCache cache;
        cache.set_budget_bytes(50);
        for (uint64_t i = 0; i < 20; ++i) {
            cache.store_property(i, static_cast<double>(i));
        }
        check(cache.current_usage_bytes() <= 50, "Usage stays under budget after many stores");
        cache.clear();
        check(cache.current_usage_bytes() == 0, "Usage is zero after clear");
    }

    return g_failures == 0;
}
