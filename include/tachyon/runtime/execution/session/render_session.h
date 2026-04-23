#pragma once

#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/media/playback_scheduler.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"
#include "tachyon/runtime/execution/presentation_clock.h"
#include "tachyon/runtime/execution/framebuffer_playback_queue.h"

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
    media::MediaPrefetcher m_prefetcher;
    std::unique_ptr<media::PlaybackScheduler> m_scheduler;
    
    std::unique_ptr<runtime::RuntimeSurfacePool> m_surface_pool;
    std::unique_ptr<runtime::PresentationClock> m_clock;
    std::unique_ptr<runtime::FramebufferPlaybackQueue> m_playback_queue;

    std::optional<std::size_t> m_memory_budget_bytes;
};

} // namespace tachyon
