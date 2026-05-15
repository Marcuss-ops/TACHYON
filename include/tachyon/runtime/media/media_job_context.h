#pragma once

#include <filesystem>

namespace tachyon::runtime::media {

/**
 * @brief Context for a media processing job, managing working directories and intermediate files.
 */
struct MediaJobContext {
    /// Directory where intermediate files (segments, audio extraction) are stored.
    std::filesystem::path work_dir;
    
    /// The final output path for the rendered media.
    std::filesystem::path output_path;
    
    /// Whether to keep intermediate files after the job completes.
    bool keep_intermediates{false};
    
    /**
     * @brief Creates a context with a specific work directory.
     * If work_dir is empty, a temporary directory should be used.
     */
    static MediaJobContext create_default(const std::filesystem::path& output) {
        MediaJobContext ctx;
        ctx.output_path = output;
        ctx.work_dir = output.parent_path() / ".tachyon_work";
        return ctx;
    }
};

} // namespace tachyon::runtime::media
