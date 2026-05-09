#pragma once

#include "tachyon/text/layout/layout.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::text {

struct LayoutCacheKey {
    std::string text;
    std::uint64_t font_id{0}; 
    std::uint32_t pixel_size{0};
    std::vector<TextFeature> features;
    TextBoxSpec box;
    TextLayoutOptions options;

    LayoutCacheKey() = default;

    bool operator==(const LayoutCacheKey& other) const {
        return text == other.text &&
               font_id == other.font_id &&
               pixel_size == other.pixel_size &&
               box.width == other.box.width &&
               box.height == other.box.height &&
               box.mode == other.box.mode &&
               box.horizontal_align == other.box.horizontal_align &&
               box.vertical_align == other.box.vertical_align &&
               box.tracking_amount == other.box.tracking_amount &&
               box.fixed_pitch == other.box.fixed_pitch &&
               options.tracking == other.options.tracking &&
               options.word_wrap == other.options.word_wrap &&
               options.use_sdf == other.options.use_sdf &&
               options.fixed_pitch == other.options.fixed_pitch &&
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
