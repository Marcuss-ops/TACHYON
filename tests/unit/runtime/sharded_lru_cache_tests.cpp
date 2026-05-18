#include "test_utils.h"
#include "tachyon/runtime/cache/sharded_lru_cache.h"
#include <iostream>
#include <vector>
#include <thread>
#include <cassert>

using namespace tachyon;

bool run_sharded_lru_cache_tests() {
    std::cout << "[ShardedLruCacheTests] Starting ShardedLruCache unit tests...\n";

    // Test 1: Store & Lookup Base Hit/Miss Stats
    {
        ShardedLruCache<int, std::string> cache(1024 * 1024, 4); // 4 shards
        
        assert(cache.shard_count() == 4);
        
        std::string val;
        if (cache.lookup(1, val)) {
            std::cerr << "[ShardedLruCacheTests] FAIL: Expected lookup miss\n";
            return false;
        }

        cache.store(1, "one", 100);
        
        if (!cache.lookup(1, val) || val != "one") {
            std::cerr << "[ShardedLruCacheTests] FAIL: Expected lookup hit\n";
            return false;
        }

        auto cs = cache.stats();
        if (cs.hits != 1 || cs.misses != 1 || cs.lookups != 2) {
            std::cerr << "[ShardedLruCacheTests] FAIL: Cache stats mismatch\n";
            return false;
        }
        std::cout << "[ShardedLruCacheTests] Base hit/miss stats verified!\n";
    }

    // Test 2: Capacity Eviction Distribution Across Shards
    {
        // 4 shards, total capacity 400 bytes -> 100 bytes per shard
        ShardedLruCache<int, std::string> cache(400, 4);
        
        // Let's store elements to specifically evict on certain shards
        for (int i = 0; i < 20; ++i) {
            cache.store(i, "val", 60); // 60 bytes each
        }

        // Each shard has 100 bytes capacity. A single element is 60 bytes.
        // Two elements would be 120 bytes (exceeds shard budget).
        // Therefore, each shard can hold at most 1 element at a time!
        // So after storing 20 elements, current usage should be at most 4 * 60 = 240 bytes.
        if (cache.current_usage_bytes() > 240) {
            std::cerr << "[ShardedLruCacheTests] FAIL: Eviction did not bound memory. Usage: " 
                      << cache.current_usage_bytes() << "\n";
            return false;
        }
        std::cout << "[ShardedLruCacheTests] Capacity eviction bounding verified!\n";
    }

    // Test 3: Update Existing Key
    {
        ShardedLruCache<int, std::string> cache(1000, 2);
        cache.store(1, "one", 100);
        cache.store(1, "updated_one", 150); // Updates value and size

        std::string val;
        if (!cache.lookup(1, val) || val != "updated_one") {
            std::cerr << "[ShardedLruCacheTests] FAIL: Update failed to replace value\n";
            return false;
        }

        if (cache.current_usage_bytes() != 150) {
            std::cerr << "[ShardedLruCacheTests] FAIL: Update failed to correct capacity. Usage: " 
                      << cache.current_usage_bytes() << "\n";
            return false;
        }
        std::cout << "[ShardedLruCacheTests] Key updating verified!\n";
    }

    // Test 4: Custom Move Semantics
    {
        ShardedLruCache<int, std::string> cache(1000, 2);
        cache.store(1, "one", 100);
        cache.store(2, "two", 100);

        // Move construct
        ShardedLruCache<int, std::string> cache2(std::move(cache));

        std::string val;
        if (!cache2.lookup(1, val) || val != "one") {
            std::cerr << "[ShardedLruCacheTests] FAIL: Move constructor lost elements\n";
            return false;
        }
        if (!cache2.lookup(2, val) || val != "two") {
            std::cerr << "[ShardedLruCacheTests] FAIL: Move constructor lost elements\n";
            return false;
        }
        std::cout << "[ShardedLruCacheTests] Move safety verified!\n";
    }

    // Test 5: Parallel Contention Safety
    {
        ShardedLruCache<int, int> cache(1024 * 1024, 8);
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t) {
            threads.emplace_back([&cache, t]() {
                for (int i = 0; i < 100; ++i) {
                    int key = t * 1000 + i;
                    cache.store(key, i, sizeof(int));
                    int val = 0;
                    cache.lookup(key, val);
                }
            });
        }

        for (auto& th : threads) {
            th.join();
        }

        std::cout << "[ShardedLruCacheTests] Parallel multi-threaded safety verified!\n";
    }

    // Test 6: Shard stats and distribution
    {
        ShardedLruCache<int, int> cache(1000, 4);
        for (int i = 0; i < 100; ++i) {
            cache.store(i, i, 10);
        }
        
        auto s_stats = cache.shard_stats();
        std::size_t total_bytes = 0;
        for (const auto& s : s_stats) {
            total_bytes += s.bytes_used;
        }
        
        if (total_bytes != cache.current_usage_bytes()) {
            std::cerr << "[ShardedLruCacheTests] FAIL: Shard stats bytes mismatch\n";
            return false;
        }
        std::cout << "[ShardedLruCacheTests] Shard stats distribution verified!\n";
    }

    // Test 7: Store same key multiple times updates size correctly
    {
        ShardedLruCache<int, int> cache(1000, 1); // 1 shard for simplicity
        cache.store(1, 10, 100);
        if (cache.current_usage_bytes() != 100) return false;
        
        cache.store(1, 20, 250);
        if (cache.current_usage_bytes() != 250) {
            std::cerr << "[ShardedLruCacheTests] FAIL: Sequential store same key size mismatch. Got " 
                      << cache.current_usage_bytes() << "\n";
            return false;
        }
        std::cout << "[ShardedLruCacheTests] Size update on existing key verified!\n";
    }

    std::cout << "[ShardedLruCacheTests] ALL TESTS PASSED!\n";
    return true;
}
