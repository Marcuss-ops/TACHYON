#pragma once

#include "tachyon/core/media/media_types.h"
#include <filesystem>
#include <optional>
#include <string>

namespace tachyon::runtime::media {

/**
 * @brief Persistent cache for media probe results.
 */
class ProbeCache {
public:
    explicit ProbeCache(const std::filesystem::path& cache_dir);
    
    std::optional<core::media::FullMetadata> get(const std::filesystem::path& path);
    void put(const std::filesystem::path& path, const core::media::FullMetadata& meta);

private:
    std::filesystem::path get_cache_path(const std::filesystem::path& path) const;
    std::filesystem::path cache_dir_;
};

} // namespace tachyon::runtime::media
