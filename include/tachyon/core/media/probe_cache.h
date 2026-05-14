#pragma once

#include "tachyon/core/media/probe.h"
#include <string>
#include <map>
#include <mutex>
#include <filesystem>
#include <list>

namespace tachyon::media {

/**
 * @brief Persistent cache for media metadata with LRU eviction.
 * Derived from ruststream-core/probe/cache.rs
 */
class ProbeCache {
public:
    /**
     * @brief Retrieves the singleton instance of the probe cache.
     */
    static ProbeCache& instance();

    /**
     * @brief Gets metadata from cache or probes the file if not present.
     * @param path Path to the media file.
     * @return A MediaResult containing FullMetadata or a MediaError.
     */
    core::MediaResult<FullMetadata> get_or_probe(const std::filesystem::path& path);
    
    /**
     * @brief Manually adds metadata to the cache.
     */
    void put(const std::string& path, const FullMetadata& meta);

    /**
     * @brief Clears all entries from the cache.
     */
    void clear();
    
    /**
     * @brief Sets the maximum number of entries before LRU eviction starts.
     */
    void set_capacity(size_t max_entries);

    struct Stats {
        size_t hits{0};
        size_t misses{0};
        size_t entry_count{0};
    };

    /**
     * @brief Retrieves cache performance statistics.
     */
    Stats get_stats() const;

private:
    ProbeCache() = default;
    
    mutable std::mutex m_mutex;
    
    // LRU Implementation: map for fast lookup, list for tracking usage order
    std::map<std::string, std::pair<FullMetadata, std::list<std::string>::iterator>> m_cache;
    std::list<std::string> m_lru_order;
    
    size_t m_max_entries{1000};
    mutable Stats m_stats;

    void evict_if_needed();
};

} // namespace tachyon::media
