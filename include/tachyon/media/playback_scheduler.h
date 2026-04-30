#pragma once

#include "tachyon/media/management/media_manager.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <set>

namespace tachyon::media {

/**
 * @brief Prioritizes and schedules media decoding tasks.
 */
class PlaybackScheduler {
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

    PlaybackScheduler(MediaManager& manager, std::size_t thread_count = 4) 
        : m_manager(manager), m_running(true) {
        for (std::size_t i = 0; i < thread_count; ++i) {
            m_workers.emplace_back([this]() { worker_loop(); });
        }
    }
    
    ~PlaybackScheduler() {
        m_running = false;
        m_cv.notify_all();
        for (auto& worker : m_workers) {
            if (worker.joinable()) worker.join();
        }
    }
    
    void schedule_task(Task task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            // Basic deduplication: if exact task exists, just update priority if higher
            // In a real NLE, we'd use a set or map for O(log N) lookup
            // We can't easily iterate a priority_queue, so we use a secondary tracking set or just push
            // For MVP deduplication, we'll just push and let the cache handle the second hit as a no-op
            // BUT to satisfy the "refinement" request, I'll add a simple tracking set.
            
            std::string key = task.asset_id + "@" + std::to_string(task.time);
            if (m_scheduled_keys.find(key) != m_scheduled_keys.end()) {
                return; 
            }
            m_scheduled_keys.insert(key);
            m_queue.push(std::move(task));
        }
        m_cv.notify_one();
    }

    void clear_queue() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_queue.empty()) m_queue.pop();
            m_scheduled_keys.clear();
        }
    }

    void update() {
        // Polling if needed, but workers handle the queue
    }

private:
    void worker_loop() {
        while (m_running) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]() { return !m_queue.empty() || !m_running; });
                if (!m_running && m_queue.empty()) return;
                task = std::move(m_queue.top());
                m_queue.pop();
                m_scheduled_keys.erase(task.asset_id + "@" + std::to_string(task.time));
            }

            // Execute decoding task
            auto resolved_path = m_manager.resolve_media_path(task.asset_id);
            if (!resolved_path.empty()) {
                auto* decoder = m_manager.acquire_video_decoder(resolved_path);
                if (decoder) {
                    auto frame = decoder->get_frame_at_time(task.time);
                    if (frame) {
                        m_manager.store_video_frame(
                            task.asset_id,
                            task.time,
                            std::make_unique<renderer2d::SurfaceRGBA>(*frame));
                    }
                    m_manager.release_video_decoder(resolved_path, decoder);
                }
            }
        }
    }

    MediaManager& m_manager;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;
    std::vector<std::thread> m_workers;
    std::set<std::string> m_scheduled_keys;
    struct TaskCompare {
        bool operator()(const Task& a, const Task& b) {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority); // High int = high priority
        }
    };
    std::priority_queue<Task, std::vector<Task>, TaskCompare> m_queue;
};

} // namespace tachyon::media
