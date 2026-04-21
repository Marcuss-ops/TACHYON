#pragma once

#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/runtime/core/diagnostics.h"
#include "tachyon/renderer2d/render_context.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

struct RenderSessionResult {
    std::vector<ExecutedFrame> frames;
    std::size_t cache_hits{0};
    std::size_t cache_misses{0};
    std::size_t frames_written{0};
    bool output_configured{false};
    std::string output_error;
    DiagnosticBag diagnostics;
    std::vector<FrameDiagnostics> frame_diagnostics;
};

class RenderSession {
public:
    RenderSessionResult render(const SceneSpec& scene, const CompiledScene& compiled_scene, const RenderExecutionPlan& execution_plan, const std::filesystem::path& output_path = {});
    RenderSessionResult render(
        const SceneSpec& scene,
        const CompiledScene& compiled_scene,
        const RenderExecutionPlan& execution_plan,
        const std::filesystem::path& output_path,
        std::size_t worker_count);

    void set_memory_budget_bytes(std::size_t bytes) { m_memory_budget_bytes = bytes; }

    FrameCache& cache() { return m_cache; }
    const FrameCache& cache() const { return m_cache; }
    std::shared_ptr<renderer2d::PrecompCache> precomp_cache() { return m_precomp_cache; }
    std::shared_ptr<const renderer2d::PrecompCache> precomp_cache() const { return m_precomp_cache; }

private:
    FrameCache m_cache;
    std::shared_ptr<renderer2d::PrecompCache> m_precomp_cache{std::make_shared<renderer2d::PrecompCache>()};
    std::optional<std::size_t> m_memory_budget_bytes;
};

} // namespace tachyon
