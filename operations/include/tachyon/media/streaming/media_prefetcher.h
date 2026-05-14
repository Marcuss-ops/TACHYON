#pragma once

#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/core/media/media_provider.h"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <mutex>

namespace tachyon::media {

/**
 * @brief Predicts and pre-fetches media frames to ensure smooth playback.
 */
class MediaPrefetcher : public IMediaPrefetcher {
public:
    void update(
        IMediaProvider& manager, 
        IPlaybackScheduler& scheduler, 
        const std::vector<std::string>& active_video_paths, 
        double current_time, 
        double fps, 
        int prefetch_count = 5) override;

private:
    std::map<std::string, std::vector<double>> m_requested;
    double m_last_time{-1.0};
    std::mutex m_mutex;
};

} // namespace tachyon::media
