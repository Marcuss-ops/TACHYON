#pragma once

#include "tachyon/text/layout/layout.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::text {

struct LayoutCacheKey {
    std::string text;
    std::uint64_t font_id; // For simplicity, we'll use the primary font ID or a hash of the chain
    std::uint32_t pixel_size;
    std::vector<TextFeature> features;
    std::uint32_t box_width;
    std::uint32_t box_height;
    bool multiline;
    TextAlignment alignment;
    float tracking;
    bool word_wrap;
    bool use_sdf;

    bool operator==(const LayoutCacheKey& other) const {
        return text == other.text &&
               font_id == other.font_id &&
               pixel_size == other.pixel_size &&
               box_width == other.box_width &&
               box_height == other.box_height &&
               multiline == other.multiline &&
               alignment == other.alignment &&
               tracking == other.tracking &&
               word_wrap == other.word_wrap &&
               use_sdf == other.use_sdf &&
               features.size() == other.features.size() &&
               std::equal(features.begin(), features.end(), other.features.begin(),
                    [](const auto& a, const auto& b) { return a.tag == b.tag && a.value == b.value; });
    }
};

struct LayoutCacheKeyHash {
    std::size_t operator()(const LayoutCacheKey& key) const;
};

class LayoutCache {
public:
    LayoutCache();
    ~LayoutCache();

    LayoutCache(const LayoutCache&) = delete;
    LayoutCache& operator=(const LayoutCache&) = delete;

    static LayoutCache& get_instance();

    bool get(const LayoutCacheKey& key, TextLayoutResult& out_result) const;
    void put(const LayoutCacheKey& key, const TextLayoutResult& result);
    void clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<LayoutCacheKey, TextLayoutResult, LayoutCacheKeyHash> m_cache;
};

} // namespace tachyon::text
