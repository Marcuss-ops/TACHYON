#pragma once
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include <vector>

namespace tachyon {

/**
 * @brief Internal parallel rendering engine.
 * Supports both buffered and streaming modes.
 */
void render_frames_parallel_internal(
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    std::size_t worker_count,
    ::tachyon::RenderContext& context,
    media::MediaPrefetcher& prefetcher,
    media::PlaybackScheduler* scheduler,
    std::vector<ExecutedFrame>& rendered_frames,
    RenderProgressCallback progress_callback = nullptr,
    CancelFlag* cancel_flag = nullptr,
    runtime::DiskCacheStore* disk_cache = nullptr,
    output::FrameOutputSink* sink = nullptr,
    RenderSessionResult* result = nullptr,
    std::vector<double>* frame_times_out = nullptr);

} // namespace tachyon
