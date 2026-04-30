#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/media/playback_scheduler.h"
#include <algorithm>

namespace tachyon::media {

void MediaPrefetcher::update(MediaManager& manager, PlaybackScheduler& scheduler, const std::vector<std::string>& active_video_paths, double current_time, double fps, int prefetch_count) {
    if (fps <= 0.0) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (std::abs(current_time - m_last_time) > 1.0) {
        // Discontinuity detected (seek)
        scheduler.clear_queue();
        m_requested.clear();
    }
    m_last_time = current_time;
    for (auto it = m_requested.begin(); it != m_requested.end(); ) {
        if (std::find(active_video_paths.begin(), active_video_paths.end(), it->first) == active_video_paths.end()) {
            it = m_requested.erase(it);
        } else {
            // Remove timestamps too far in the past
            auto& times = it->second;
            times.erase(std::remove_if(times.begin(), times.end(), [current_time](double t) {
                return t < current_time - 1.0; 
            }), times.end());
            ++it;
        }
    }

    for (const auto& path : active_video_paths) {
        auto& requested = m_requested[path];
        
        std::filesystem::path resolved = manager.resolve_media_path(path);
        if (resolved.empty()) continue;

        auto decoder = manager.acquire_video_decoder(resolved);
        if (!decoder) continue;
        const double duration = decoder->duration();

        for (int i = 1; i <= prefetch_count; ++i) {
            double prefetch_time = current_time + (static_cast<double>(i) / fps);
            if (prefetch_time > duration) break;

            bool already_requested = std::any_of(requested.begin(), requested.end(), [prefetch_time](double t) {
                return std::abs(t - prefetch_time) < 0.001;
            });

            if (!already_requested) {
                requested.push_back(prefetch_time);
                scheduler.schedule_task({path, prefetch_time, PlaybackScheduler::Priority::Prefetch});
            }
        }
    }
}

} // namespace tachyon::media
