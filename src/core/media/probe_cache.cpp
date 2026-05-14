#include "tachyon/core/media/probe_cache.h"

namespace tachyon::media {

ProbeCache& ProbeCache::instance() {
    static ProbeCache inst;
    return inst;
}

core::MediaResult<FullMetadata> ProbeCache::get_or_probe(const std::filesystem::path& path) {
    std::string key = path.string();
    
    std::unique_lock<std::mutex> lock(m_mutex);
    
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.hits++;
        // Move to front of LRU
        m_lru_order.erase(it->second.second);
        m_lru_order.push_front(key);
        it->second.second = m_lru_order.begin();
        return core::MediaResult<FullMetadata>::success(it->second.first);
    }
    
    m_stats.misses++;
    
    // Unlock during probe to allow other threads to access cache while we do I/O
    lock.unlock();
    auto probe_res = MediaProbe::probe_file(path);
    lock.lock();
    
    // Re-check if someone else put it in while we were probing
    if (m_cache.find(key) == m_cache.end() && probe_res.ok()) {
        m_lru_order.push_front(key);
        m_cache[key] = {*probe_res.value, m_lru_order.begin()};
        evict_if_needed();
    }
    
    return probe_res;
}

void ProbeCache::put(const std::string& path, const FullMetadata& meta) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        m_lru_order.erase(it->second.second);
    }
    
    m_lru_order.push_front(path);
    m_cache[path] = {meta, m_lru_order.begin()};
    
    evict_if_needed();
}

void ProbeCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_lru_order.clear();
}

void ProbeCache::set_capacity(size_t max_entries) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_max_entries = max_entries;
    evict_if_needed();
}

ProbeCache::Stats ProbeCache::get_stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    Stats s = m_stats;
    s.entry_count = m_cache.size();
    return s;
}

void ProbeCache::evict_if_needed() {
    while (m_cache.size() > m_max_entries && !m_lru_order.empty()) {
        std::string oldest_key = m_lru_order.back();
        m_lru_order.pop_back();
        m_cache.erase(oldest_key);
    }
}

} // namespace tachyon::media
