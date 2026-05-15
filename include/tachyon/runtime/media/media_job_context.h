#pragma once

#include <filesystem>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

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
     * @brief Creates a context with a unique work directory to avoid collisions.
     */
    static MediaJobContext create_default(const std::filesystem::path& output) {
        MediaJobContext ctx;
        ctx.output_path = output;
        
        // Generate a simple unique job ID
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::stringstream ss;
        ss << ".tachyon_work_" << std::hex << ms;
        
        ctx.work_dir = output.parent_path() / ss.str();
        return ctx;
    }
};

} // namespace tachyon::runtime::media
