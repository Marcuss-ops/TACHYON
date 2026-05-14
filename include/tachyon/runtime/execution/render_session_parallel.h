#pragma once

#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/output/frame_output_sink.h"

#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace tachyon {

// Forward declarations
struct FrameQueue;
void render_frames_parallel(
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    std::size_t worker_count,
    ::tachyon::RenderContext& context,
    media::MediaPrefetcher& prefetcher,
    media::PlaybackScheduler* scheduler,
    const CompiledScene& original_scene,
    double session_fps,
    std::vector<ExecutedFrame>& rendered_frames,
    RenderProgressCallback progress_callback = nullptr,
    CancelFlag* cancel_flag = nullptr,
    runtime::DiskCacheStore* disk_cache = nullptr,
    output::FrameOutputSink* sink = nullptr,
    RenderSessionResult* result = nullptr
);

} // namespace tachyon
