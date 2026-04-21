#include "tachyon/runtime/cache/frame_cache.h"

#include <algorithm>

namespace tachyon {

namespace {
std::size_t estimate_layer_size(const scene::EvaluatedLayerState& layer) {
    std::size_t size = sizeof(scene::EvaluatedLayerState);
    size += layer.id.capacity();
    size += layer.blend_mode.capacity();
    size += layer.text_content.capacity();
    size += layer.font_id.capacity();
    if (layer.subtitle_path) size += layer.subtitle_path->capacity();
    if (layer.shape_path.has_value()) {
        size += layer.shape_path->points.capacity() * sizeof(scene::EvaluatedShapePathPoint);
    }
    size += layer.effects.capacity() * sizeof(EffectSpec);
    return size;
}

std::size_t estimate_comp_size(const scene::EvaluatedCompositionState& comp) {
    std::size_t size = sizeof(scene::EvaluatedCompositionState);
    size += comp.layers.capacity() * sizeof(std::shared_ptr<scene::EvaluatedLayerState>);
    return size;
}

std::size_t estimate_frame_size(const renderer2d::Framebuffer& frame) {
    return sizeof(renderer2d::Framebuffer) + (frame.pixels().capacity() * sizeof(float));
}

void erase_from_order(std::vector<std::uint64_t>& order, std::uint64_t key) {
    const auto it = std::find(order.begin(), order.end(), key);
    if (it != order.end()) {
        order.erase(it);
    }
}
} // namespace

bool FrameCache::lookup_property(std::uint64_t key, double& out_value) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        out_value = it->second;
        ++m_hit_count;
        const_cast<FrameCache*>(this)->touch(key);
        return true;
    }
    ++m_miss_count;
    return false;
}

void FrameCache::store_property(std::uint64_t key, double value) {
    std::scoped_lock lock(m_mutex);
    const std::size_t size = sizeof(double);
    if (m_entries.contains(key)) {
        remove_entry(key);
    }

    m_properties[key] = value;
    m_current_usage_bytes += size;
    m_entries[key] = EntryInfo{EntryType::Property, size};
    m_lru_order.push_back(key);
    evict_if_needed();
}

std::shared_ptr<const scene::EvaluatedLayerState> FrameCache::lookup_layer(std::uint64_t key) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_layers.find(key);
    if (it != m_layers.end()) {
        ++m_hit_count;
        const_cast<FrameCache*>(this)->touch(key);
        return it->second;
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::store_layer(std::uint64_t key, std::shared_ptr<const scene::EvaluatedLayerState> state) {
    if (!state) return;
    std::scoped_lock lock(m_mutex);
    const std::size_t size = estimate_layer_size(*state);
    if (m_entries.contains(key)) {
        remove_entry(key);
    }

    m_layers.insert_or_assign(key, state);
    m_current_usage_bytes += size;
    m_entries[key] = EntryInfo{EntryType::Layer, size};
    m_lru_order.push_back(key);
    evict_if_needed();
}

std::shared_ptr<const scene::EvaluatedCompositionState> FrameCache::lookup_composition(std::uint64_t key) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_compositions.find(key);
    if (it != m_compositions.end()) {
        ++m_hit_count;
        const_cast<FrameCache*>(this)->touch(key);
        return it->second;
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::store_composition(std::uint64_t key, std::shared_ptr<const scene::EvaluatedCompositionState> state) {
    if (!state) return;
    std::scoped_lock lock(m_mutex);
    const std::size_t size = estimate_comp_size(*state);
    if (m_entries.contains(key)) {
        remove_entry(key);
    }

    m_compositions.insert_or_assign(key, state);
    m_current_usage_bytes += size;
    m_entries[key] = EntryInfo{EntryType::Composition, size};
    m_lru_order.push_back(key);
    evict_if_needed();
}

std::shared_ptr<const renderer2d::Framebuffer> FrameCache::lookup_frame(std::uint64_t key) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_frames.find(key);
    if (it != m_frames.end()) {
        ++m_hit_count;
        const_cast<FrameCache*>(this)->touch(key);
        return it->second;
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::store_frame(std::uint64_t key, std::shared_ptr<const renderer2d::Framebuffer> frame) {
    if (!frame) return;
    std::scoped_lock lock(m_mutex);
    const std::size_t size = estimate_frame_size(*frame);
    if (m_entries.contains(key)) {
        remove_entry(key);
    }

    m_frames.insert_or_assign(key, frame);
    m_current_usage_bytes += size;
    m_entries[key] = EntryInfo{EntryType::Frame, size};
    m_lru_order.push_back(key);
    evict_if_needed();
}

void FrameCache::touch(std::uint64_t key) {
    const auto it = std::find(m_lru_order.begin(), m_lru_order.end(), key);
    if (it != m_lru_order.end()) {
        m_lru_order.erase(it);
        m_lru_order.push_back(key);
    }
}

void FrameCache::set_budget_bytes(std::size_t bytes) {
    std::scoped_lock lock(m_mutex);
    m_max_budget_bytes = bytes;
    evict_if_needed();
}

void FrameCache::evict_if_needed() {
    if (m_max_budget_bytes == 0) {
        return;
    }

    while (m_current_usage_bytes > m_max_budget_bytes && !m_lru_order.empty()) {
        const std::uint64_t oldest = m_lru_order.front();
        remove_entry(oldest);
    }
}

void FrameCache::remove_entry(std::uint64_t key) {
    const auto entry_it = m_entries.find(key);
    if (entry_it == m_entries.end()) {
        return;
    }

    switch (entry_it->second.type) {
        case EntryType::Property: m_properties.erase(key); break;
        case EntryType::Layer: m_layers.erase(key); break;
        case EntryType::Composition: m_compositions.erase(key); break;
        case EntryType::Frame: m_frames.erase(key); break;
    }

    m_current_usage_bytes -= entry_it->second.size;
    m_entries.erase(entry_it);
    erase_from_order(m_lru_order, key);
}

std::size_t FrameCache::current_usage_bytes() const {
    std::scoped_lock lock(m_mutex);
    return m_current_usage_bytes;
}

void FrameCache::store(const CachedFrame& frame) {
    std::scoped_lock lock(m_mutex);
    m_legacy_frames.push_back(frame);
}

CachedFrame* FrameCache::lookup(const FrameCacheKey& key, std::uint64_t scene_hash) {
    std::scoped_lock lock(m_mutex);
    for (auto& frame : m_legacy_frames) {
        if (frame.scene_hash == scene_hash && frame_cache_entry_matches(frame.entry, key)) {
            ++m_hit_count;
            return &frame;
        }
    }
    ++m_miss_count;
    return nullptr;
}

const CachedFrame* FrameCache::lookup(const FrameCacheKey& key, std::uint64_t scene_hash) const {
    std::scoped_lock lock(m_mutex);
    for (const auto& frame : m_legacy_frames) {
        if (frame.scene_hash == scene_hash && frame_cache_entry_matches(frame.entry, key)) {
            ++m_hit_count;
            return &frame;
        }
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::invalidate(const std::string& dependency) {
    std::scoped_lock lock(m_mutex);
    m_legacy_frames.erase(
        std::remove_if(m_legacy_frames.begin(), m_legacy_frames.end(), [&](const CachedFrame& frame) {
            return std::find(frame.invalidates_when_changed.begin(), frame.invalidates_when_changed.end(), dependency) != frame.invalidates_when_changed.end();
        }),
        m_legacy_frames.end());
}


void FrameCache::clear() {
    std::scoped_lock lock(m_mutex);
    m_properties.clear();
    m_layers.clear();
    m_compositions.clear();
    m_frames.clear();
    m_legacy_frames.clear();
    m_entries.clear();
    m_lru_order.clear();
    m_current_usage_bytes = 0;
    m_hit_count = 0;
    m_miss_count = 0;
}

} // namespace tachyon
