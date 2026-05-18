#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <list>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <functional>

namespace tachyon {

/**
 * @brief Generic thread-safe Sharded LRU Cache.
 * 
 * Partitions elements across multiple lock-boundaries (shards) based on the key hash.
 * This significantly reduces lock contention in highly parallel rendering environments.
 */
template<class Key, class Value, class Hash = std::hash<Key>>
class ShardedLruCache {
public:
    struct CacheStats {
        std::size_t hits{0};
        std::size_t misses{0};
        std::size_t lookups{0};
        std::size_t bytes_used{0};
    };

    explicit ShardedLruCache(std::size_t capacity_bytes, std::size_t shard_count = 64)
        : shards_(shard_count) {
        std::size_t cap_per_shard = capacity_bytes / shard_count;
        if (cap_per_shard == 0) cap_per_shard = 1;
        for (auto& shard : shards_) {
            shard.capacity_bytes = cap_per_shard;
        }
    }

    ~ShardedLruCache() = default;

    // Delete copy semantics
    ShardedLruCache(const ShardedLruCache&) = delete;
    ShardedLruCache& operator=(const ShardedLruCache&) = delete;

    // Explicit move safety
    ShardedLruCache(ShardedLruCache&& other) noexcept {
        shards_ = std::move(other.shards_);
        hash_ = std::move(other.hash_);
    }

    ShardedLruCache& operator=(ShardedLruCache&& other) noexcept {
        if (this == &other) return *this;
        shards_ = std::move(other.shards_);
        hash_ = std::move(other.hash_);
        return *this;
    }

    /**
     * @brief Lookup value and assign it to reference. Returns true if hit.
     */
    bool lookup(const Key& key, Value& out_value) const {
        std::size_t idx = hash_(key) % shards_.size();
        auto& shard = const_cast<Shard&>(shards_[idx]);
        std::lock_guard<std::mutex> lock(shard.mutex);

        shard.lookups++;
        auto it = shard.map.find(key);
        if (it != shard.map.end()) {
            shard.hits++;
            shard.lru.splice(shard.lru.begin(), shard.lru, it->second.list_it);
            out_value = it->second.value;
            return true;
        }
        shard.misses++;
        return false;
    }

    /**
     * @brief Lookup value directly. Returns default-constructed Value (e.g., nullptr) if miss.
     */
    Value lookup(const Key& key) const {
        Value val{};
        lookup(key, val);
        return val;
    }

    /**
     * @brief Store value with associated size estimation.
     */
    void store(const Key& key, Value value, std::size_t size_bytes) {
        std::size_t idx = hash_(key) % shards_.size();
        auto& shard = shards_[idx];
        std::lock_guard<std::mutex> lock(shard.mutex);

        auto it = shard.map.find(key);
        if (it != shard.map.end()) {
            shard.bytes_used -= it->second.size_bytes;
            it->second.value = std::move(value);
            it->second.size_bytes = size_bytes;
            shard.lru.splice(shard.lru.begin(), shard.lru, it->second.list_it);
        } else {
            shard.lru.push_front(key);
            shard.map[key] = Entry{std::move(value), size_bytes, shard.lru.begin()};
        }
        shard.bytes_used += size_bytes;
        evict_if_needed(shard);
    }

    void clear() {
        for (auto& shard : shards_) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            shard.map.clear();
            shard.lru.clear();
            shard.bytes_used = 0;
            shard.hits = 0;
            shard.misses = 0;
            shard.lookups = 0;
        }
    }

    void set_capacity_bytes(std::size_t bytes) {
        std::size_t cap_per_shard = bytes / shards_.size();
        if (cap_per_shard == 0) cap_per_shard = 1;
        for (auto& shard : shards_) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            shard.capacity_bytes = cap_per_shard;
            evict_if_needed(shard);
        }
    }

    [[nodiscard]] std::size_t hit_count() const noexcept {
        std::size_t total = 0;
        for (const auto& shard : shards_) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            total += shard.hits;
        }
        return total;
    }

    [[nodiscard]] std::size_t miss_count() const noexcept {
        std::size_t total = 0;
        for (const auto& shard : shards_) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            total += shard.misses;
        }
        return total;
    }

    [[nodiscard]] std::size_t lookup_count() const noexcept {
        std::size_t total = 0;
        for (const auto& shard : shards_) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            total += shard.lookups;
        }
        return total;
    }

    [[nodiscard]] std::size_t current_usage_bytes() const noexcept {
        std::size_t total = 0;
        for (const auto& shard : shards_) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            total += shard.bytes_used;
        }
        return total;
    }

private:
    struct Entry {
        Value value;
        std::size_t size_bytes{0};
        typename std::list<Key>::iterator list_it;
    };

    struct Shard {
        mutable std::mutex mutex;
        std::unordered_map<Key, Entry, Hash> map;
        std::list<Key> lru;
        std::size_t bytes_used{0};
        std::size_t capacity_bytes{0};
        std::size_t hits{0};
        std::size_t misses{0};
        std::size_t lookups{0};

        Shard() = default;

        // Delete copy constructor & assignment due to std::mutex
        Shard(const Shard&) = delete;
        Shard& operator=(const Shard&) = delete;

        // Thread-safe custom move constructor
        Shard(Shard&& other) noexcept {
            std::scoped_lock lock(other.mutex);
            map = std::move(other.map);
            lru = std::move(other.lru);
            bytes_used = other.bytes_used;
            capacity_bytes = other.capacity_bytes;
            hits = other.hits;
            misses = other.misses;
            lookups = other.lookups;
            rebuild_lru_iterators();
        }

        // Thread-safe custom move assignment
        Shard& operator=(Shard&& other) noexcept {
            if (this == &other) return *this;
            std::scoped_lock lock(mutex, other.mutex);
            map = std::move(other.map);
            lru = std::move(other.lru);
            bytes_used = other.bytes_used;
            capacity_bytes = other.capacity_bytes;
            hits = other.hits;
            misses = other.misses;
            lookups = other.lookups;
            rebuild_lru_iterators();
            return *this;
        }

        void rebuild_lru_iterators() {
            for (auto it = lru.begin(); it != lru.end(); ++it) {
                auto found = map.find(*it);
                if (found != map.end()) {
                    found->second.list_it = it;
                }
            }
        }
    };

    void evict_if_needed(Shard& shard) {
        while (shard.bytes_used > shard.capacity_bytes && !shard.lru.empty()) {
            const auto& lru_key = shard.lru.back();
            auto it = shard.map.find(lru_key);
            if (it != shard.map.end()) {
                shard.bytes_used -= it->second.size_bytes;
                shard.map.erase(it);
            }
            shard.lru.pop_back();
        }
    }

    std::vector<Shard> shards_;
    Hash hash_;
};

} // namespace tachyon
