#pragma once

#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/media/management/playback_scheduler.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"
#include "tachyon/runtime/execution/presentation_clock.h"
#include "tachyon/runtime/execution/framebuffer_playback_queue.h"
#include "tachyon/runtime/cache/disk_cache.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <atomic>

namespace tachyon {

using RenderProgressCallback = std::function<void(std::size_t frame_index, std::size_t total_frames)>;
using CancelFlag = std::atomic<bool>;

struct RenderSessionResult {
    std::vector<ExecutedFrame> frames;
    std::size_t cache_hits{0};
    std::size_t cache_misses{0};
    std::size_t frames_written{0};
    bool output_configured{false};
    std::string output_error;
    std::string audio_output_path; // For muxing audio later
    DiagnosticBag diagnostics;
    std::vector<FrameDiagnostics> frame_diagnostics;

    // Timing metrics
    double wall_time_total_ms{0.0};
    double wall_time_per_frame_ms{0.0};
    double encode_time_ms{0.0};

    // Scene metadata
    std::string scene_id;
    std::string composition_id;
    double duration_seconds{0.0};
    std::size_t frame_count{0};
    int width{0};
    int height{0};
    double fps_target{24.0};
    double cache_hit_rate{0.0};

    // System metrics
    std::size_t peak_memory_bytes{0};
    std::size_t thread_count{1};
    int quality_tier{1};

    // Timestamp
    std::string timestamp_iso8601;
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

    // Render with progress callback
    RenderSessionResult render(
        const SceneSpec& scene,
        const CompiledScene& compiled_scene,
        const RenderExecutionPlan& execution_plan,
        const std::filesystem::path& output_path,
        std::size_t worker_count,
        RenderProgressCallback progress_callback);

    // Render with progress callback and cancellation support
    RenderSessionResult render(
        const SceneSpec& scene,
        const CompiledScene& compiled_scene,
        const RenderExecutionPlan& execution_plan,
        const std::filesystem::path& output_path,
        std::size_t worker_count,
        RenderProgressCallback progress_callback,
        CancelFlag& cancel_flag);

    // Set disk cache for checkpoint/resume support
    void set_disk_cache(std::unique_ptr<runtime::DiskCacheStore> disk_cache) { m_disk_cache = std::move(disk_cache); }
    runtime::DiskCacheStore* disk_cache() const { return m_disk_cache.get(); }

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

    std::unique_ptr<runtime::DiskCacheStore> m_disk_cache;

    std::optional<std::size_t> m_memory_budget_bytes;
};

} // namespace tachyon

