#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/core/render_graph.h"
#include "tachyon/core/scene/evaluated_state.h"
#include "tachyon/runtime/execution/render_plan.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {

/**
 * @brief Multi-level cache for incremental rendering.
 * 
 * Stores evaluation results at different granularities to maximize reuse
 * when only small parts of the scene change.
 */
class FrameCache {
public:
    // Property Level
    bool lookup_property(std::uint64_t key, double& out_value) const;
    void store_property(std::uint64_t key, double value);

    // Layer Level
    std::shared_ptr<const scene::EvaluatedLayerState> lookup_layer(std::uint64_t key) const;
    void store_layer(std::uint64_t key, std::shared_ptr<const scene::EvaluatedLayerState> state);

    // Composition Level
    std::shared_ptr<const scene::EvaluatedCompositionState> lookup_composition(std::uint64_t key) const;
    void store_composition(std::uint64_t key, std::shared_ptr<const scene::EvaluatedCompositionState> state);

    // Final Frame Level
    std::shared_ptr<const renderer2d::Framebuffer> lookup_frame(std::uint64_t key) const;
    void store_frame(std::uint64_t key, std::shared_ptr<const renderer2d::Framebuffer> frame);

    void clear();

    [[nodiscard]] std::size_t hit_count() const noexcept { return m_hit_count; }
    [[nodiscard]] std::size_t miss_count() const noexcept { return m_miss_count; }

    void set_budget_bytes(std::size_t bytes);
    void evict_if_needed();
    [[nodiscard]] std::size_t current_usage_bytes() const;

    // Legacy compatibility surface for older tests.
    void store(const CachedFrame& frame);
    CachedFrame* lookup(const FrameCacheKey& key, std::uint64_t scene_hash);
    const CachedFrame* lookup(const FrameCacheKey& key, std::uint64_t scene_hash) const;
    void invalidate(const std::string& dependency);

private:
    mutable std::mutex m_mutex;
    mutable std::size_t m_hit_count{0};
    mutable std::size_t m_miss_count{0};

    std::unordered_map<std::uint64_t, double> m_properties;
    std::unordered_map<std::uint64_t, std::shared_ptr<const scene::EvaluatedLayerState>> m_layers;
    std::unordered_map<std::uint64_t, std::shared_ptr<const scene::EvaluatedCompositionState>> m_compositions;
    std::unordered_map<std::uint64_t, std::shared_ptr<const renderer2d::Framebuffer>> m_frames;
    std::vector<CachedFrame> m_legacy_frames;

    enum class EntryType { Property, Layer, Composition, Frame };
    struct EntryInfo {
        EntryType type;
        std::size_t size;
    };
    std::unordered_map<std::uint64_t, EntryInfo> m_entries;
    std::vector<std::uint64_t> m_lru_order; // front = oldest

    std::size_t m_max_budget_bytes{1024ULL * 1024 * 1024}; // 1GB default
    std::size_t m_current_usage_bytes{0};

    void touch(std::uint64_t key);
    void remove_entry(std::uint64_t key);
};


} // namespace tachyon
