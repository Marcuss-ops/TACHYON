#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <map>

namespace tachyon::runtime {

class DiskCacheStore;

/**
 * @brief PlaybackEngine manages real-time playback and pre-fetching.
 */
class PlaybackEngine {
public:
    struct FrameRequest {
        int64_t frame_number;
        double time_seconds;
        int priority; // lower is higher priority (0 = current)
    };

    PlaybackEngine(DiskCacheStore* disk_cache = nullptr);
    ~PlaybackEngine();

    void start();
    void stop();

    /**
     * @brief Requests a frame to be rendered/loaded.
     */
    void request_frame(const FrameRequest& req);

    /**
     * @brief Gets a rendered frame if available.
     */
    std::shared_ptr<renderer2d::SurfaceRGBA> get_frame(int64_t frame_number);

    /**
     * @brief Sets the current playback position to focus pre-fetching.
     */
    void set_current_position(int64_t frame_number);

private:
    void worker_loop();

    DiskCacheStore* m_disk_cache;
    std::atomic<bool> m_running{false};
    std::thread m_worker;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    
    struct CompareRequest {
        bool operator()(const FrameRequest& a, const FrameRequest& b) {
            return a.priority > b.priority; // higher priority first
        }
    };
    std::priority_queue<FrameRequest, std::vector<FrameRequest>, CompareRequest> m_queue;
    
    std::map<int64_t, std::shared_ptr<renderer2d::SurfaceRGBA>> m_ram_cache;
    int64_t m_current_frame{0};
};

} // namespace tachyon::runtime
