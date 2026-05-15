#pragma once

#include "tachyon/core/media/media_types.h"
#include "tachyon/core/types/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::core::media {

/**
 * @brief Utility for probing media files and extracting metadata.
 */
class MediaProbe {
public:
    /**
     * @brief Probes a file and returns its metadata.
     */
    static MediaResult<FullMetadata> probe_file(const std::filesystem::path& path);
    
    /**
     * @brief Performs a full probe with deep metadata extraction.
     */
    static MediaResult<FullMetadata> probe_full(const std::filesystem::path& path);
};

} // namespace tachyon::core::media
