#include "tachyon/text/layout/layout_cache.h"
#include <functional>

namespace tachyon::text {

namespace {
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace

std::size_t LayoutCacheKeyHash::operator()(const LayoutCacheKey& key) const {
    std::size_t seed = 0;
    hash_combine(seed, key.text);
    hash_combine(seed, key.font_id);
    hash_combine(seed, key.pixel_size);
    hash_combine(seed, key.box_width);
    hash_combine(seed, key.box_height);
    hash_combine(seed, key.multiline);
    hash_combine(seed, (int)key.alignment);
    hash_combine(seed, key.tracking);
    hash_combine(seed, key.word_wrap);
    hash_combine(seed, key.use_sdf);
    for (const auto& f : key.features) {
        hash_combine(seed, f.tag);
        hash_combine(seed, f.value);
    }
    return seed;
}

LayoutCache::LayoutCache() = default;
LayoutCache::~LayoutCache() = default;

LayoutCache& LayoutCache::get_instance() {
    static LayoutCache instance;
    return instance;
}

bool LayoutCache::get(const LayoutCacheKey& key, TextLayoutResult& out_result) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        out_result = it->second;
        return true;
    }
    return false;
}

void LayoutCache::put(const LayoutCacheKey& key, const TextLayoutResult& result) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Limit cache size to prevent memory leaks if text is constantly changing
    if (m_cache.size() > 1000) {
        m_cache.clear();
    }
    m_cache[key] = result;
}

void LayoutCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

} // namespace tachyon::text
