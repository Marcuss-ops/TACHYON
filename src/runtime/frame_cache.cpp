#include "tachyon/runtime/frame_cache.h"

namespace tachyon {

const CachedFrame* FrameCache::lookup(const FrameCacheKey& key, const std::string& scene_signature) const {
    std::scoped_lock lock(m_mutex);
    const auto it = m_entries.find(key.value);
    if (it == m_entries.end()) {
        ++m_miss_count;
        return nullptr;
    }

    if (it->second.scene_signature != scene_signature) {
        ++m_miss_count;
        return nullptr;
    }

    ++m_hit_count;
    return &it->second;
}

void FrameCache::store(CachedFrame frame) {
    std::scoped_lock lock(m_mutex);
    m_entries.insert_or_assign(frame.entry.key.value, std::move(frame));
}

void FrameCache::invalidate(const std::string& changed_parameter) {
    std::scoped_lock lock(m_mutex);
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        bool should_remove = false;
        for (const auto& token : it->second.invalidates_when_changed) {
            if (token == changed_parameter) {
                should_remove = true;
                break;
            }
        }

        if (should_remove) {
            it = m_entries.erase(it);
        } else {
            ++it;
        }
    }
}

void FrameCache::clear() {
    std::scoped_lock lock(m_mutex);
    m_entries.clear();
}

} // namespace tachyon
