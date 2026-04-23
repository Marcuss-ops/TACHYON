#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <filesystem>

namespace tachyon::runtime {

/**
 * @brief Identifies a specific render result for caching.
 */
struct CacheKey {
    std::uint64_t identity{0};
    std::string node_path;       ///< Subgraph or layer identifier.
    double frame_time{0.0};
    int quality_tier{1};         ///< 0=draft, 1=preview, 2=final.
    std::string aov_type{"beauty"}; ///< "beauty", "depth", "motion_vectors", etc.
    
    [[nodiscard]] std::string to_filename() const;
};

/**
 * @brief Persistent disk cache for rendered frames.
 * 
 * Rules:
 * - Frames are stored as chunked binary data (e.g. compressed RGBA).
 * - Key must include all variables that affect visible output.
 * - Invalidation is incremental based on node path prefixes.
 */
class DiskCacheStore {
public:
    explicit DiskCacheStore(std::filesystem::path root_dir);

    /**
     * @brief Store frame data in the disk cache.
     */
    void store(const CacheKey& key, const std::vector<uint8_t>& frame_data);

    /**
     * @brief Load frame data from the disk cache.
     */
    [[nodiscard]] std::optional<std::vector<uint8_t>> load(const CacheKey& key);

    /**
     * @brief Invalidate cache entries matching a node path prefix.
     */
    void invalidate(const std::string& node_path_prefix);

    /**
     * @brief Purge old entries to stay under a disk budget.
     */
    void purge_to_budget(std::size_t max_bytes);

    struct Stats {
        std::size_t hits{0};
        std::size_t misses{0};
        std::size_t total_size_bytes{0};
        std::size_t entry_count{0};
    };

    [[nodiscard]] Stats get_stats() const;

private:
    std::filesystem::path m_root;
    mutable Stats m_stats;
};

} // namespace tachyon::runtime