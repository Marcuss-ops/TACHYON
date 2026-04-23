#include "tachyon/runtime/playback/ram_playback_queue.h"

namespace tachyon::runtime {

RamPlaybackQueue::RamPlaybackQueue(std::size_t max_memory_bytes) 
    : m_max_memory(max_memory_bytes) {}

void RamPlaybackQueue::push_frame(Frame frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames[frame.frame_number] = std::move(frame);
    enforce_memory_limit();
}

std::optional<RamPlaybackQueue::Frame> RamPlaybackQueue::get_frame(std::int64_t frame_number) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_frames.find(frame_number);
    if (it != m_frames.end()) {
        return it->second;
    }
    return std::nullopt;
}

void RamPlaybackQueue::purge_outside_window(std::int64_t current_frame, std::int64_t window_size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_frames.begin(); it != m_frames.end(); ) {
        if (std::abs(it->first - current_frame) > window_size) {
            it = m_frames.erase(it);
        } else {
            ++it;
        }
    }
}

void RamPlaybackQueue::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames.clear();
}

void RamPlaybackQueue::enforce_memory_limit() {
    std::size_t current_usage = 0;
    for (const auto& [num, frame] : m_frames) {
        current_usage += frame.data.size();
    }

    // Industrial eviction: remove furthest from some presumed "current" point
    // For simplicity, here we just remove oldest/youngest entries if we are over budget
    while (current_usage > m_max_memory && !m_frames.empty()) {
        // Remove the entry furthest from the middle of the current set
        auto start = m_frames.begin()->first;
        auto end = m_frames.rbegin()->first;
        auto mid = (start + end) / 2;

        if (std::abs(start - mid) > std::abs(end - mid)) {
            current_usage -= m_frames.begin()->second.data.size();
            m_frames.erase(m_frames.begin());
        } else {
            auto it = m_frames.end();
            --it;
            current_usage -= it->second.data.size();
            m_frames.erase(it);
        }
    }
}

} // namespace tachyon::runtime
