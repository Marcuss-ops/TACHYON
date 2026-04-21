#include "tachyon/runtime/cache/frame_cache.h"

namespace tachyon {

bool FrameCache::lookup_property(std::uint64_t key, double& out_value) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        out_value = it->second;
        ++m_hit_count;
        return true;
    }
    ++m_miss_count;
    return false;
}

void FrameCache::store_property(std::uint64_t key, double value) {
    std::scoped_lock lock(m_mutex);
    m_properties[key] = value;
}

std::shared_ptr<const scene::EvaluatedLayerState> FrameCache::lookup_layer(std::uint64_t key) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_layers.find(key);
    if (it != m_layers.end()) {
        ++m_hit_count;
        return it->second;
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::store_layer(std::uint64_t key, std::shared_ptr<scene::EvaluatedLayerState> state) {
    std::scoped_lock lock(m_mutex);
    m_layers.insert_or_assign(key, std::move(state));
}

std::shared_ptr<const scene::EvaluatedCompositionState> FrameCache::lookup_composition(std::uint64_t key) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_compositions.find(key);
    if (it != m_compositions.end()) {
        ++m_hit_count;
        return it->second;
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::store_composition(std::uint64_t key, std::shared_ptr<scene::EvaluatedCompositionState> state) {
    std::scoped_lock lock(m_mutex);
    m_compositions.insert_or_assign(key, std::move(state));
}

std::shared_ptr<const renderer2d::Framebuffer> FrameCache::lookup_frame(std::uint64_t key) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_frames.find(key);
    if (it != m_frames.end()) {
        ++m_hit_count;
        return it->second;
    }
    ++m_miss_count;
    return nullptr;
}

void FrameCache::store_frame(std::uint64_t key, std::shared_ptr<renderer2d::Framebuffer> frame) {
    std::scoped_lock lock(m_mutex);
    m_frames.insert_or_assign(key, std::move(frame));
}


void FrameCache::clear() {
    std::scoped_lock lock(m_mutex);
    m_properties.clear();
    m_layers.clear();
    m_compositions.clear();
    m_frames.clear();
    m_hit_count = 0;
    m_miss_count = 0;
}

} // namespace tachyon

