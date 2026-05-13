#pragma once

#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/media/playback_scheduler.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include "tachyon/runtime/execution/presentation_clock.h"
#include "tachyon/runtime/execution/framebuffer_playback_queue.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/runtime/execution/compiled_frame_program.h"
#include "tachyon/runtime/policy/worker_budget.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

namespace runtime { struct RuntimeRegistryBundle; }
namespace presets { class TextRegistry; }
namespace profiling { class RenderProfiler; }
    
/**
 * @brief Progress callback for long-running render operations.
 * First argument is the number of completed frames, second is total frames.
 */
using RenderProgressCallback = std::function<void(std::size_t, std::size_t)>;

/**
 * @brief Thread-safe flag used to request cancellation of a render session.
 */
using CancelFlag = std::atomic<bool>;

struct RenderSessionResult {
    std::vector<ExecutedFrame> frames;
    std::size_t cache_hits{0};
    std::size_t cache_misses{0};
    std::size_t frames_written{0};
    bool output_configured{false};
    std::string output_error;
    DiagnosticBag diagnostics;
    std::vector<FrameDiagnostics> frame_diagnostics;

    // Phase timings (milliseconds)
    double scene_compile_ms{0.0};
    double plan_build_ms{0.0};
    double execution_plan_build_ms{0.0};
    double frame_execution_ms{0.0};
    double encode_ms{0.0};
    double io_read_ms{0.0};
    double io_write_ms{0.0};

    // Aggregate timings
    double wall_time_total_ms{0.0};
    double wall_time_per_frame_ms{0.0};

    // Memory
    std::size_t peak_working_set_bytes{0};
    std::size_t avg_working_set_bytes{0};
    std::size_t peak_private_bytes{0};
    std::size_t avg_private_bytes{0};
    double avg_cpu_percent_machine{0.0};
    double avg_cpu_cores_used{0.0};

    // I/O
    std::size_t input_bytes{0};
    std::size_t output_bytes{0};

    // Per-frame timings (for avg/peak calculation)
    std::vector<double> frame_times_ms;

    // Cache hit rate (calculated)
    double cache_hit_rate() const {
        const std::size_t total = cache_hits + cache_misses;
        return total > 0 ? (static_cast<double>(cache_hits) / total) * 100.0 : 0.0;
    }
};


class RenderSession {
public:
    RenderSession();
    RenderSessionResult render(const SceneSpec& scene, const CompiledScene& compiled_scene, const RenderExecutionPlan& execution_plan, const std::filesystem::path& output_path = {});
    RenderSessionResult render(
        const SceneSpec& scene,
        const CompiledScene& compiled_scene,
        const RenderExecutionPlan& execution_plan,
        const std::filesystem::path& output_path,
        const ::tachyon::runtime::RenderWorkerBudget& budget = ::tachyon::runtime::RenderWorkerBudget{},
        CancelFlag* cancel_flag = nullptr);
    // 100x performance: render using precompiled frame program
    RenderSessionResult render(const CompiledFrameProgram& program, double time_sec, const std::filesystem::path& output_path = {});

    void set_memory_budget_bytes(std::size_t bytes) { m_memory_budget_bytes = bytes; }
    void set_profiler(profiling::RenderProfiler* profiler) { m_profiler = profiler; }
    void set_transition_registry(const TransitionRegistry* registry) { m_transition_registry_ptr = registry; }
    void set_text_registry(const presets::TextRegistry* registry) { m_text_registry_ptr = registry; }
    void set_registry_bundle(const runtime::RuntimeRegistryBundle* bundle);

    FrameCache& cache() { return m_cache; }
    const FrameCache& cache() const { return m_cache; }
    std::shared_ptr<renderer2d::PrecompCache> precomp_cache() { return m_precomp_cache; }
    const std::shared_ptr<renderer2d::PrecompCache>& precomp_cache() const { return m_precomp_cache; }

private:
    FrameCache m_cache;
    std::shared_ptr<renderer2d::PrecompCache> m_precomp_cache{std::make_shared<renderer2d::PrecompCache>()};
    std::shared_ptr<renderer2d::PrecompCache> m_text_surface_cache{std::make_shared<renderer2d::PrecompCache>()};
    media::MediaPrefetcher m_prefetcher;
    std::unique_ptr<media::PlaybackScheduler> m_scheduler;
    
    std::shared_ptr<SurfacePool> m_surface_pool{std::make_shared<SurfacePool>()};
    std::unique_ptr<runtime::PresentationClock> m_clock;
    std::unique_ptr<runtime::FramebufferPlaybackQueue> m_playback_queue;

    std::unique_ptr<runtime::RuntimeRegistryBundle> m_bundle;

    std::optional<std::size_t> m_memory_budget_bytes;
    profiling::RenderProfiler* m_profiler{nullptr};
    const runtime::RuntimeRegistryBundle* m_bundle_ptr{nullptr};
    const TransitionRegistry* m_transition_registry_ptr{nullptr};
    const presets::TextRegistry* m_text_registry_ptr{nullptr};
};

} // namespace tachyon
