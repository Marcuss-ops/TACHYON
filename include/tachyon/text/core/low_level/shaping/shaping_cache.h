#pragma once

#include "tachyon/text/core/low_level/shaping/font_shaping.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "tachyon/text/layout/layout.h"

namespace tachyon::renderer2d::text::shaping {

struct ShapingCacheKey {
    std::uint64_t font_id;
    std::uint64_t content_hash;
    std::uint32_t scale;
    int direction;
    std::string script;
    std::string language;
    std::vector<std::uint32_t> codepoints;
    std::vector<tachyon::text::TextFeature> features;

    bool operator==(const ShapingCacheKey& other) const {
        return font_id == other.font_id &&
               content_hash == other.content_hash &&
               scale == other.scale &&
               direction == other.direction &&
               script == other.script &&
               language == other.language &&
               codepoints == other.codepoints &&
               features.size() == other.features.size() &&
               std::equal(features.begin(), features.end(), other.features.begin(), 
                    [](const auto& a, const auto& b) { return a.tag == b.tag && a.value == b.value; });
    }
};

struct ShapingCacheKeyHash {
    std::size_t operator()(const ShapingCacheKey& key) const;
};

class ShapingCache {
public:
    ShapingCache();
    ~ShapingCache();

    ShapingCache(const ShapingCache&) = delete;
    ShapingCache& operator=(const ShapingCache&) = delete;

    static ShapingCache& get_instance();

    // Returns a copy of the run to ensure thread safety
    bool get(const ShapingCacheKey& key, ShapedGlyphRun& out_run) const;
    void put(const ShapingCacheKey& key, const ShapedGlyphRun& run);
    void clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<ShapingCacheKey, ShapedGlyphRun, ShapingCacheKeyHash> m_cache;
};

} // namespace tachyon::renderer2d::text::shaping
