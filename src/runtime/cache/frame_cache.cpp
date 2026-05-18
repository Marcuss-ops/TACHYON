#include "tachyon/runtime/cache/frame_cache.h"
#include <algorithm>

namespace tachyon {

namespace {

std::size_t estimate_layer_size(const scene::EvaluatedLayerState& layer) {
    std::size_t size = sizeof(scene::EvaluatedLayerState);
    
    // strings
    size += layer.identity.id.capacity();
    size += layer.identity.layer_id.capacity();
    size += layer.identity.name.capacity();
    size += layer.transform.blend_mode.capacity();
    size += layer.text.content.capacity();
    size += layer.text.font_id.capacity();
    
    // vectors
    if (layer.vector.shape_path.has_value()) {
        size += layer.vector.shape_path->points.capacity() * sizeof(scene::EvaluatedShapePoint);
    }
    
    // collections
    size += layer.effects.capacity() * sizeof(EvaluatedEffect);
    size += layer.text.animators.capacity() * sizeof(TextAnimatorSpec);
    size += layer.text.highlights.capacity() * sizeof(TextHighlightSpec);
    size += layer.masks.list.capacity() * sizeof(spec::MaskPath);
    
    return size;
}

std::size_t estimate_comp_size(const scene::EvaluatedCompositionState& comp) {
    std::size_t size = sizeof(scene::EvaluatedCompositionState);
    size += comp.composition_id.capacity();
    size += comp.layers.capacity() * sizeof(scene::EvaluatedLayerState);
    for (const auto& layer : comp.layers) {
        size += estimate_layer_size(layer);
    }
    return size;
}

std::size_t estimate_frame_size(const renderer2d::Framebuffer& frame) {
    return sizeof(renderer2d::Framebuffer) + (frame.pixels().capacity() * sizeof(float));
}

} // namespace

bool FrameCache::lookup_property(std::uint64_t key, double& out_value) const {
    return m_properties.lookup(key, out_value);
}

void FrameCache::store_property(std::uint64_t key, double value) {
    m_properties.store(key, value, sizeof(double));
}

std::shared_ptr<const scene::EvaluatedLayerState> FrameCache::lookup_layer(std::uint64_t key) const {
    return m_layers.lookup(key);
}

void FrameCache::store_layer(std::uint64_t key, std::shared_ptr<const scene::EvaluatedLayerState> state) {
    if (!state) return;
    m_layers.store(key, state, estimate_layer_size(*state));
}

std::shared_ptr<const scene::EvaluatedCompositionState> FrameCache::lookup_composition(std::uint64_t key) const {
    return m_compositions.lookup(key);
}

void FrameCache::store_composition(std::uint64_t key, std::shared_ptr<const scene::EvaluatedCompositionState> state) {
    if (!state) return;
    m_compositions.store(key, state, estimate_comp_size(*state));
}

std::shared_ptr<const renderer2d::Framebuffer> FrameCache::lookup_frame(const FrameCacheKey& key) const {
    FrameEntry entry;
    if (m_frames.lookup(key.hash, entry)) {
        if (entry.key_value == key.value) {
            return entry.framebuffer;
        }
    }
    return nullptr;
}

void FrameCache::store_frame(const FrameCacheKey& key, std::shared_ptr<const renderer2d::Framebuffer> frame) {
    if (!frame) return;
    m_frames.store(key.hash, FrameEntry{key.value, frame}, estimate_frame_size(*frame));
}

std::shared_ptr<const renderer2d::Framebuffer> FrameCache::lookup_frame(std::uint64_t key) const {
    FrameEntry entry;
    if (m_frames.lookup(key, entry)) {
        return entry.framebuffer;
    }
    return nullptr;
}

void FrameCache::store_frame(std::uint64_t key, std::shared_ptr<const renderer2d::Framebuffer> frame) {
    if (!frame) return;
    m_frames.store(key, FrameEntry{"", frame}, estimate_frame_size(*frame));
}

void FrameCache::set_budget_bytes(std::size_t bytes) {
    m_max_budget_bytes = bytes;
    
    // Redistribute budget dynamically:
    // Properties: 5% (min 1MB)
    // Layers: 15% (min 5MB)
    // Compositions: 15% (min 5MB)
    // Frames: remaining 65%
    std::size_t prop_budget = std::max<std::size_t>(1024 * 1024, bytes * 0.05);
    std::size_t layer_budget = std::max<std::size_t>(5 * 1024 * 1024, bytes * 0.15);
    std::size_t comp_budget = std::max<std::size_t>(5 * 1024 * 1024, bytes * 0.15);
    std::size_t frame_budget = (bytes > (prop_budget + layer_budget + comp_budget))
        ? (bytes - (prop_budget + layer_budget + comp_budget))
        : (10 * 1024 * 1024);

    m_properties.set_capacity_bytes(prop_budget);
    m_layers.set_capacity_bytes(layer_budget);
    m_compositions.set_capacity_bytes(comp_budget);
    m_frames.set_capacity_bytes(frame_budget);
}

void FrameCache::evict_if_needed() {
    // No-op: Auto-eviction is handled inside individual ShardedLruCaches during store operations
}

std::size_t FrameCache::current_usage_bytes() const {
    return m_properties.current_usage_bytes() + 
           m_layers.current_usage_bytes() + 
           m_compositions.current_usage_bytes() + 
           m_frames.current_usage_bytes();
}

std::size_t FrameCache::hit_count() const noexcept {
    return m_properties.hit_count() + 
           m_layers.hit_count() + 
           m_compositions.hit_count() + 
           m_frames.hit_count();
}

std::size_t FrameCache::miss_count() const noexcept {
    return m_properties.miss_count() + 
           m_layers.miss_count() + 
           m_compositions.miss_count() + 
           m_frames.miss_count();
}

void FrameCache::clear() {
    m_properties.clear();
    m_layers.clear();
    m_compositions.clear();
    m_frames.clear();
}

} // namespace tachyon
