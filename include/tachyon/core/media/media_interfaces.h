#pragma once

#include "tachyon/core/media/media_types.h"
#include "tachyon/core/media/media_error.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace tachyon::media {

class IMediaProvider;

/**
 * @brief Interface for prioritizing and scheduling media decoding tasks.
 */
class IPlaybackScheduler {
public:
    enum class Priority {
        RealTime,   // Immediate playhead
        Prefetch,   // Future frames
        Proxy       // Background proxy gen
    };

    struct Task {
        std::string asset_id;
        double time;
        Priority priority;
    };

    virtual ~IPlaybackScheduler() = default;
    virtual void schedule_task(Task task) = 0;
    virtual void clear_queue() = 0;
    virtual void update() = 0;
};

/**
 * @brief Interface for predicting and pre-fetching media frames.
 */
class IMediaPrefetcher {
public:
    virtual ~IMediaPrefetcher() = default;
    virtual void update(
        IMediaProvider& manager, 
        IPlaybackScheduler& scheduler, 
        const std::vector<std::string>& active_video_paths, 
        double current_time, 
        double fps, 
        int prefetch_count = 5) = 0;
};

} // namespace tachyon::media

namespace tachyon::core::media {

/**
 * @brief Interface for probing media file metadata.
 */
class IMediaProbe {
public:
    virtual ~IMediaProbe() = default;
    virtual MediaResult<FullMetadata> probe_file(const std::filesystem::path& path) = 0;
    virtual MediaResult<FullMetadata> probe_full(const std::filesystem::path& path) = 0;
};

/**
 * @brief Interface for audio analysis (e.g. waveform, loudness, transcription).
 */
class IAudioAnalyzer {
public:
    virtual ~IAudioAnalyzer() = default;
    virtual MediaResult<std::vector<float>> analyze_waveform(const std::filesystem::path& path) = 0;
    virtual MediaResult<std::string> transcribe(const std::filesystem::path& path) = 0;
};

/**
 * @brief Interface for video encoding.
 */
class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    virtual MediaResult<void> open(const std::filesystem::path& output_path, int width, int height, double fps) = 0;
    virtual MediaResult<void> write_frame(const renderer2d::SurfaceRGBA& surface) = 0;
    virtual void close() = 0;
};

/**
 * @brief Interface for applying procedural transitions between frames.
 */
class ITransitionRenderer {
public:
    virtual ~ITransitionRenderer() = default;

    /**
     * @brief Renders a transition frame.
     */
    virtual MediaResult<renderer2d::SurfaceRGBA> render(
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        float progress
    ) = 0;
};

} // namespace tachyon::core::media
