#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/core/graph/runtime_render_graph.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/sharded_lru_cache.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
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
    std::shared_ptr<const renderer2d::Framebuffer> lookup_frame(const FrameCacheKey& key) const;
    void store_frame(const FrameCacheKey& key, std::shared_ptr<const renderer2d::Framebuffer> frame);

    // Old overloads for internal use / simple keys
    std::shared_ptr<const renderer2d::Framebuffer> lookup_frame(std::uint64_t hash) const;
    void store_frame(std::uint64_t hash, std::shared_ptr<const renderer2d::Framebuffer> frame);

    void clear();

    [[nodiscard]] std::size_t hit_count() const noexcept;
    [[nodiscard]] std::size_t miss_count() const noexcept;

    void set_budget_bytes(std::size_t bytes);
    void evict_if_needed();
    [[nodiscard]] std::size_t current_usage_bytes() const;

    struct FrameEntry {
        std::string key_value;
        std::shared_ptr<const renderer2d::Framebuffer> framebuffer;
    };

private:
    ShardedLruCache<std::uint64_t, double> m_properties{50 * 1024 * 1024}; // 50MB
    ShardedLruCache<std::uint64_t, std::shared_ptr<const scene::EvaluatedLayerState>> m_layers{150 * 1024 * 1024}; // 150MB
    ShardedLruCache<std::uint64_t, std::shared_ptr<const scene::EvaluatedCompositionState>> m_compositions{150 * 1024 * 1024}; // 150MB
    ShardedLruCache<std::uint64_t, FrameEntry> m_frames{674 * 1024 * 1024}; // 674MB

    std::size_t m_max_budget_bytes{1024ULL * 1024 * 1024}; // 1GB default
};


} // namespace tachyon
