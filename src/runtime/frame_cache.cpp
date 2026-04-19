#include "tachyon/runtime/frame_cache.h"

namespace tachyon {

const CachedFrame* FrameCache::lookup(const FrameCacheKey& key, const std::string& state_fingerprint) const {
    const auto it = m_entries.find(key.value);
    if (it == m_entries.end()) {
        ++m_miss_count;
        return nullptr;
    }

    if (it->second.state_fingerprint != state_fingerprint) {
        ++m_miss_count;
        return nullptr;
    }

    ++m_hit_count;
    return &it->second;
}

void FrameCache::store(CachedFrame frame) {
    m_entries[frame.entry.key.value] = std::move(frame);
}

void FrameCache::invalidate(const std::string& changed_parameter) {
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
    m_entries.clear();
}

} // namespace tachyon
