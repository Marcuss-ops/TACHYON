#pragma once

#include "tachyon/core/media/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::ops {

/**
 * @brief Simplified audio operations for external consumers.
 * Delegating all logic to the Tachyon Core Audio system.
 */
class AudioOps {
public:
    /**
     * @brief Transcribe an audio file using Tachyon's core audio engine.
     */
    static core::MediaResult<std::string> transcribe(const std::filesystem::path& path);
};

} // namespace tachyon::ops
