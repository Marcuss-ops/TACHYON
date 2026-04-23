#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <deque>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace tachyon::runtime {

/**
 * @brief A thread-safe queue for pre-rendered frames.
 * Functions as a ring-buffer for the playback consumer.
 */
class FramebufferPlaybackQueue {
public:
    explicit FramebufferPlaybackQueue(size_t max_size = 10) : m_max_size(max_size) {}

    /**
     * @brief Pushes a pre-rendered frame into the queue.
     * Blocks if the queue is full.
     */
    void push(std::uint64_t frame_number, std::shared_ptr<renderer2d::Framebuffer> frame) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_full.wait(lock, [this]() { return m_queue.size() < m_max_size; });
        
        m_queue.push_back({frame_number, std::move(frame)});
        m_cv_empty.notify_one();
    }

    /**
     * @brief Pops the next ready frame.
     * Blocks if the queue is empty.
     */
    std::pair<std::uint64_t, std::shared_ptr<renderer2d::Framebuffer>> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_empty.wait(lock, [this]() { return !m_queue.empty(); });
        
        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        
        m_cv_full.notify_one();
        return item;
    }

    /**
     * @brief Non-blocking check for the next frame.
     */
    bool peek_frame_number(std::uint64_t& out_frame_number) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        out_frame_number = m_queue.front().first;
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
        m_cv_full.notify_all();
    }

private:
    size_t m_max_size;
    std::deque<std::pair<std::uint64_t, std::shared_ptr<renderer2d::Framebuffer>>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv_empty;
    std::condition_variable m_cv_full;
};

} // namespace tachyon::runtime