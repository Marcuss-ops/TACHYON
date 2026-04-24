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
 * 
 * Must include ALL fields that affect visible output to ensure cache correctness.
 */
struct CacheKey {
    std::uint64_t identity{0};
    std::string node_path;       ///< Subgraph or layer identifier.
    double frame_time{0.0};
    int quality_tier{1};         ///< 0=draft, 1=preview, 2=final.
    std::string aov_type{"beauty"}; ///< "beauty", "depth", "motion_vectors", etc.
    
    // Temporal effects fields (must be included in cache identity)
    spec::TimeRemapMode time_remap_mode{spec::TimeRemapMode::Blend};
    spec::FrameBlendMode frame_blend_mode{spec::FrameBlendMode::Linear};
    float frame_blend_strength{1.0f};
    
    // Motion blur parameters
    float shutter_angle_deg{180.0f};
    int motion_blur_samples{8};
    std::uint32_t motion_blur_seed{0};
    
    // Proxy settings
    bool use_proxy{false};
    std::string proxy_variant_id;
    
    [[nodiscard]] std::string to_filename() const;
    
    /// Build a complete cache key including all temporal and quality settings
    static CacheKey build(
        std::uint64_t identity,
        const std::string& node_path,
        double frame_time,
        int quality_tier = 1,
        const std::string& aov_type = "beauty",
        const spec::TimeRemapMode time_remap = spec::TimeRemapMode::Blend,
        const spec::FrameBlendMode blend_mode = spec::FrameBlendMode::Linear,
        float blend_strength = 1.0f,
        float shutter_angle = 180.0f,
        int mb_samples = 8,
        std::uint32_t mb_seed = 0,
        bool proxy = false,
        const std::string& proxy_id = ""
    );
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