#pragma once

#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/graph/compiled_node.h"
#include "tachyon/runtime/core/graph/compiled_scene.h"
#include "tachyon/runtime/execution/frame_render_task.h"

#include <optional>
#include <cstdint>

namespace tachyon::runtime {

class FrameCacheKeyFactory {
public:
    explicit FrameCacheKeyFactory(const CompiledScene& scene, const RenderPlan& plan);

    [[nodiscard]] std::uint64_t composition_key() const;
    [[nodiscard]] std::uint64_t frame_key(const FrameRenderTask& task) const;
    [[nodiscard]] std::uint64_t time_sample_key(
        const FrameRenderTask& task,
        double render_time,
        std::optional<std::uint64_t> sample_index = std::nullopt
    ) const;
    [[nodiscard]] std::uint64_t node_key(std::uint64_t frame_key, const CompiledNode& node) const;

private:
    const CompiledScene& m_scene;
    const RenderPlan& m_plan;
    mutable std::optional<std::uint64_t> m_cached_composition_key;
    bool m_manifest_enabled{false};
    mutable std::string m_manifest;

public:
    void enable_manifest(bool enabled) noexcept { m_manifest_enabled = enabled; }
    [[nodiscard]] const std::string& manifest() const noexcept { return m_manifest; }
};

} // namespace tachyon::runtime
