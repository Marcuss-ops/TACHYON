#pragma once

#include <vector>
#include <string>
#include <memory>

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
