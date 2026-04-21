#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/core/render_graph.h"
#include "tachyon/core/scene/evaluated_state.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

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
    void store_layer(std::uint64_t key, std::shared_ptr<scene::EvaluatedLayerState> state);

    // Composition Level
    std::shared_ptr<const scene::EvaluatedCompositionState> lookup_composition(std::uint64_t key) const;
    void store_composition(std::uint64_t key, std::shared_ptr<scene::EvaluatedCompositionState> state);

    // Final Frame Level
    std::shared_ptr<const renderer2d::Framebuffer> lookup_frame(std::uint64_t key) const;
    void store_frame(std::uint64_t key, std::shared_ptr<renderer2d::Framebuffer> frame);

    void clear();

    [[nodiscard]] std::size_t hit_count() const noexcept { return m_hit_count; }
    [[nodiscard]] std::size_t miss_count() const noexcept { return m_miss_count; }

private:
    mutable std::mutex m_mutex;
    mutable std::size_t m_hit_count{0};
    mutable std::size_t m_miss_count{0};

    std::unordered_map<std::uint64_t, double> m_properties;
    std::unordered_map<std::uint64_t, std::shared_ptr<scene::EvaluatedLayerState>> m_layers;
    std::unordered_map<std::uint64_t, std::shared_ptr<scene::EvaluatedCompositionState>> m_compositions;
    std::unordered_map<std::uint64_t, std::shared_ptr<renderer2d::Framebuffer>> m_frames;
};


} // namespace tachyon

