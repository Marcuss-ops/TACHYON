#include "tachyon/renderer2d/text/shaping/shaping_cache.h"

namespace tachyon::renderer2d::text::shaping {

std::size_t ShapingCacheKeyHash::operator()(const ShapingCacheKey& key) const {
    std::size_t hash = 17;
    hash = hash * 31 + std::hash<std::uint64_t>()(key.font_id);
    hash = hash * 31 + std::hash<std::uint64_t>()(key.content_hash);
    hash = hash * 31 + std::hash<std::uint32_t>()(key.scale);
    hash = hash * 31 + std::hash<int>()(key.direction);
    hash = hash * 31 + std::hash<std::string>()(key.script);
    hash = hash * 31 + std::hash<std::string>()(key.language);
    
    for (std::uint32_t cp : key.codepoints) {
        hash = hash * 31 + std::hash<std::uint32_t>()(cp);
    }

    for (const auto& feat : key.features) {
        hash = hash * 31 + std::hash<std::string>()(feat.tag);
        hash = hash * 31 + std::hash<std::uint32_t>()(feat.value);
    }

    return hash;
}

ShapingCache::ShapingCache() = default;
ShapingCache::~ShapingCache() = default;

ShapingCache& ShapingCache::get_instance() {
    static ShapingCache instance;
    return instance;
}

bool ShapingCache::get(const ShapingCacheKey& key, ShapedGlyphRun& out_run) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        out_run = it->second;
        return true;
    }
    return false;
}

void ShapingCache::put(const ShapingCacheKey& key, const ShapedGlyphRun& run) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.emplace(key, run);
}

void ShapingCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

} // namespace tachyon::renderer2d::text::shaping
