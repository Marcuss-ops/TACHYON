#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/render_graph.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {

struct CachedFrame {
    FrameCacheEntry entry;
    std::string scene_signature;
    renderer2d::Framebuffer frame;
    std::vector<std::string> invalidates_when_changed;
};

class FrameCache {
public:
    const CachedFrame* lookup(const FrameCacheKey& key, const std::string& scene_signature) const;
    void store(CachedFrame frame);
    void invalidate(const std::string& changed_parameter);
    void clear();

    std::size_t size() const { return m_entries.size(); }
    std::size_t hit_count() const { return m_hit_count; }
    std::size_t miss_count() const { return m_miss_count; }

private:
    std::unordered_map<std::string, CachedFrame> m_entries;
    mutable std::mutex m_mutex;
    mutable std::size_t m_hit_count{0};
    mutable std::size_t m_miss_count{0};
};

} // namespace tachyon
