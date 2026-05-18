#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <list>
#include <mutex>

namespace tachyon {

struct NodeCacheKey {
    std::uint64_t scene_hash{0};
    std::uint64_t node_id{0};
    std::uint64_t dependency_hash{0};
    int width{0};
    int height{0};
    double time_bucket{0.0};
    std::string color_space;
    std::string quality;

    bool operator==(const NodeCacheKey& o) const {
        return scene_hash == o.scene_hash &&
               node_id == o.node_id &&
               dependency_hash == o.dependency_hash &&
               width == o.width &&
               height == o.height &&
               time_bucket == o.time_bucket &&
               color_space == o.color_space &&
               quality == o.quality;
    }
};

struct NodeCacheKeyHash {
    std::size_t operator()(const NodeCacheKey& k) const {
        std::size_t h = 17;
        h = h * 31 + std::hash<std::uint64_t>{}(k.scene_hash);
        h = h * 31 + std::hash<std::uint64_t>{}(k.node_id);
        h = h * 31 + std::hash<std::uint64_t>{}(k.dependency_hash);
        h = h * 31 + std::hash<int>{}(k.width);
        h = h * 31 + std::hash<int>{}(k.height);
        h = h * 31 + std::hash<double>{}(k.time_bucket);
        h = h * 31 + std::hash<std::string>{}(k.color_space);
        h = h * 31 + std::hash<std::string>{}(k.quality);
        return h;
    }
};

struct NodeCacheEntry {
    std::shared_ptr<const renderer2d::Framebuffer> surface;
    std::size_t size_bytes{0};
    std::list<NodeCacheKey>::iterator list_it;
};

class NodeCache {
public:
    NodeCache() = default;
    ~NodeCache() = default;

    std::shared_ptr<const renderer2d::Framebuffer> lookup(const NodeCacheKey& key);
    void store(const NodeCacheKey& key, std::shared_ptr<const renderer2d::Framebuffer> surface);
    void clear();

    void set_capacity_bytes(std::size_t bytes);

    [[nodiscard]] std::size_t capacity_bytes() const noexcept { return m_capacity_bytes; }
    [[nodiscard]] std::size_t hit_count() const noexcept { return m_hits; }
    [[nodiscard]] std::size_t lookup_count() const noexcept { return m_lookups; }
    [[nodiscard]] std::size_t miss_count() const noexcept { return m_misses; }
    [[nodiscard]] std::size_t current_usage_bytes() const noexcept { return m_bytes_used; }

private:
    void evict_if_needed();

    std::mutex m_mutex;
    std::unordered_map<NodeCacheKey, NodeCacheEntry, NodeCacheKeyHash> m_cache;
    std::list<NodeCacheKey> m_lru_list; // Front is MRU, back is LRU
    std::size_t m_lookups{0};
    std::size_t m_hits{0};
    std::size_t m_misses{0};
    std::size_t m_bytes_used{0};
    std::size_t m_capacity_bytes{1024 * 1024 * 128}; // Default 128MB
};

} // namespace tachyon

namespace std {
template <>
struct hash<tachyon::NodeCacheKey> {
    std::size_t operator()(const tachyon::NodeCacheKey& k) const {
        return tachyon::NodeCacheKeyHash{}(k);
    }
};
} // namespace std

