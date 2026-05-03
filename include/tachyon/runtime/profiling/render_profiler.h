#pragma once

#include "tachyon/runtime/profiling/profile_event.h"
#include "tachyon/runtime/profiling/profile_summary.h"

#include <mutex>
#include <vector>
#include <optional>
#include <memory>
#include <filesystem>

namespace tachyon::profiling {

class RenderProfiler {
public:
    explicit RenderProfiler(bool enabled = false);
    ~RenderProfiler() = default;

    bool enabled() const noexcept;
    void set_enabled(bool enabled) noexcept;

    std::uint64_t begin_event(
        ProfileEventType type,
        std::string name,
        std::int64_t frame_number = -1,
        std::string layer_id = {},
        std::string effect_id = {}
    );

    void end_event(std::uint64_t event_id);

    void record_cache_hit(std::string scope, std::uint64_t key);
    void record_cache_miss(std::string scope, std::uint64_t key);

    void record_memory_sample(std::size_t bytes);
    void record_frame_result(FrameProfile profile);

    ProfileSummary summarize() const;

    bool write_trace_json(const std::filesystem::path& path) const;
    bool write_chrome_trace_json(const std::filesystem::path& path) const;
    bool write_summary_json(const std::filesystem::path& path) const;

private:
    mutable std::mutex m_mutex;
    bool m_enabled{false};
    std::vector<ProfileEvent> m_events;
    std::uint64_t m_next_event_id{1};
    
    // Internal mapping of event IDs to their index in m_events for closing
    std::unordered_map<std::uint64_t, std::size_t> m_active_events;
};

class ProfileScope {
public:
    ProfileScope(
        RenderProfiler* profiler,
        ProfileEventType type,
        std::string name,
        std::int64_t frame_number = -1,
        std::string layer_id = {},
        std::string effect_id = {}
    )
        : m_profiler(profiler) {
        if (m_profiler && m_profiler->enabled()) {
            m_event_id = m_profiler->begin_event(
                type,
                std::move(name),
                frame_number,
                std::move(layer_id),
                std::move(effect_id)
            );
        }
    }

    ~ProfileScope() {
        if (m_profiler && m_event_id.has_value()) {
            m_profiler->end_event(*m_event_id);
        }
    }

    // Non-copyable
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    RenderProfiler* m_profiler{nullptr};
    std::optional<std::uint64_t> m_event_id;
};

} // namespace tachyon::profiling
