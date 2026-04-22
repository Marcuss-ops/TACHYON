#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/media/management/media_manager.h"
#include <algorithm>

namespace tachyon::media {

void MediaPrefetcher::update(MediaManager& manager, const std::vector<std::string>& active_video_paths, double current_time, double fps, int prefetch_count) {
    if (fps <= 0.0) return;

    // Remove pending requests for paths no longer active
    for (auto it = m_pending.begin(); it != m_pending.end(); ) {
        if (std::find(active_video_paths.begin(), active_video_paths.end(), it->first) == active_video_paths.end()) {
            it = m_pending.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& path : active_video_paths) {
        auto& pending = m_pending[path];
        
        // Clean up finished requests and push to manager
        pending.erase(std::remove_if(pending.begin(), pending.end(), [&](PrefetchRequest& req) {
            if (req.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto frame = req.future.get();
                if (frame.has_value()) {
                    manager.store_video_frame(req.path, req.seconds, std::make_unique<renderer2d::SurfaceRGBA>(std::move(*frame)));
                }
                return true;
            }
            return false;
        }), pending.end());

        // We need a decoder to check duration and request async
        VideoDecoder* decoder = manager.acquire_video_decoder(path);
        if (!decoder) continue;

        const double duration = decoder->duration();

        // Add new prefetch requests
        for (int i = 1; i <= prefetch_count; ++i) {
            double prefetch_time = current_time + (static_cast<double>(i) / fps);
            if (prefetch_time > duration) break;

            // Check if already pending
            bool already_requested = std::any_of(pending.begin(), pending.end(), [prefetch_time](const PrefetchRequest& req) {
                return std::abs(req.seconds - prefetch_time) < 0.001;
            });

            if (!already_requested) {
                pending.push_back({path, prefetch_time, decoder->request_frame_async(prefetch_time)});
            }
        }

        manager.release_video_decoder(path, decoder);
    }
}

} // namespace tachyon::media
