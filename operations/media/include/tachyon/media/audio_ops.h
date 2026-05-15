#pragma once

#include "tachyon/core/types/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::ops {

/**
 * @brief Simplified audio operations for external consumers.
 */
class AudioOps {
public:
    /**
     * @brief Transcribe an audio file using Tachyon's core audio engine.
     */
    static core::MediaResult<std::string> transcribe(const std::filesystem::path& path);
};

} // namespace tachyon::ops
