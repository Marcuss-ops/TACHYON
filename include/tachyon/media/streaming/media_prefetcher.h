#pragma once

#include "tachyon/media/decoding/video_decoder.h"
#include <vector>
#include <memory>
#include <map>

namespace tachyon::media {

class MediaManager;

/**
 * @brief Predicts and pre-fetches media frames to ensure smooth playback.
 */
class MediaPrefetcher {
public:
    struct PrefetchRequest {
        std::string path;
        double seconds;
        std::future<std::optional<renderer2d::SurfaceRGBA>> future;
    };

    void update(MediaManager& manager, const std::vector<std::string>& active_video_paths, double current_time, double fps, int prefetch_count = 5);

private:
    std::map<std::string, std::vector<PrefetchRequest>> m_pending;
};

} // namespace tachyon::media
