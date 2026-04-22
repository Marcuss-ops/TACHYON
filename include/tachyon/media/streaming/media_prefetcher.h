#pragma once

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <mutex>

namespace tachyon::media {

class MediaManager;
class PlaybackScheduler;

/**
 * @brief Predicts and pre-fetches media frames to ensure smooth playback.
 */
class MediaPrefetcher {
public:
    void update(
        MediaManager& manager, 
        PlaybackScheduler& scheduler, 
        const std::vector<std::string>& active_video_paths, 
        double current_time, 
        double fps, 
        int prefetch_count = 5);

private:
    std::map<std::string, std::vector<double>> m_requested;
    double m_last_time{-1.0};
    std::mutex m_mutex;
};

} // namespace tachyon::media
