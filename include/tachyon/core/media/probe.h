#pragma once

#include "tachyon/core/media/media_types.h"
#include "tachyon/core/media/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::core::media {

/**
 * @brief Contract for probing media files and extracting metadata.
 */
class IMediaProbe {
public:
    virtual ~IMediaProbe() = default;

    /**
     * @brief Probes a file and returns its metadata.
     */
    virtual MediaResult<FullMetadata> probe_file(const std::filesystem::path& path) = 0;
    
    /**
     * @brief Performs a full probe with deep metadata extraction.
     */
    virtual MediaResult<FullMetadata> probe_full(const std::filesystem::path& path) = 0;
};

} // namespace tachyon::core::media
