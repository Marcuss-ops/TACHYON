#include "tachyon/runtime/cache/node_cache.h"

namespace tachyon {

std::shared_ptr<const renderer2d::Framebuffer> NodeCache::lookup(const NodeCacheKey& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lookups++;

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_hits++;
        // Move key to front of LRU list (MRU)
        m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.list_it);
        return it->second.surface;
    }

    m_misses++;
    return nullptr;
}

void NodeCache::store(const NodeCacheKey& key, std::shared_ptr<const renderer2d::Framebuffer> surface) {
    if (!surface) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Calculate approximate size in bytes
    // Framebuffer is SurfaceRGBA. Each pixel has 4 floats (RGBA) = 16 bytes.
    std::size_t size_bytes = static_cast<std::size_t>(surface->width()) * 
                            static_cast<std::size_t>(surface->height()) * 4 * sizeof(float);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Update existing entry
        m_bytes_used -= it->second.size_bytes;
        it->second.surface = surface;
        it->second.size_bytes = size_bytes;
        
        // Move to front of LRU list
        m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.list_it);
    } else {
        // Insert new entry
        m_lru_list.push_front(key);
        NodeCacheEntry entry{surface, size_bytes, m_lru_list.begin()};
        m_cache[key] = entry;
    }

    m_bytes_used += size_bytes;
    evict_if_needed();
}

void NodeCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_lru_list.clear();
    m_bytes_used = 0;
    m_lookups = 0;
    m_hits = 0;
    m_misses = 0;
}

void NodeCache::set_capacity_bytes(std::size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_capacity_bytes = bytes;
    evict_if_needed();
}

void NodeCache::evict_if_needed() {
    while (m_bytes_used > m_capacity_bytes && !m_lru_list.empty()) {
        // Get the LRU key (from the back)
        const auto& lru_key = m_lru_list.back();
        auto it = m_cache.find(lru_key);
        if (it != m_cache.end()) {
            m_bytes_used -= it->second.size_bytes;
            m_cache.erase(it);
        }
        m_lru_list.pop_back();
    }
}

NodeCache::NodeCache(NodeCache&& other) noexcept {
    std::scoped_lock lock(other.m_mutex);
    m_cache = std::move(other.m_cache);
    m_lru_list = std::move(other.m_lru_list);
    m_lookups = other.m_lookups;
    m_hits = other.m_hits;
    m_misses = other.m_misses;
    m_bytes_used = other.m_bytes_used;
    m_capacity_bytes = other.m_capacity_bytes;
    rebuild_lru_iterators();
}

NodeCache& NodeCache::operator=(NodeCache&& other) noexcept {
    if (this == &other) return *this;
    std::scoped_lock lock(m_mutex, other.m_mutex);
    m_cache = std::move(other.m_cache);
    m_lru_list = std::move(other.m_lru_list);
    m_lookups = other.m_lookups;
    m_hits = other.m_hits;
    m_misses = other.m_misses;
    m_bytes_used = other.m_bytes_used;
    m_capacity_bytes = other.m_capacity_bytes;
    rebuild_lru_iterators();
    return *this;
}

void NodeCache::rebuild_lru_iterators() {
    for (auto it = m_lru_list.begin(); it != m_lru_list.end(); ++it) {
        auto found = m_cache.find(*it);
        if (found != m_cache.end()) {
            found->second.list_it = it;
        }
    }
}

} // namespace tachyon

