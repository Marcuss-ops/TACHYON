#pragma once
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
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

/**
 * @brief Muxes a video file and an audio file into a single file using FFmpeg.
 * The video file will be replaced with the muxed version.
 */
bool mux_audio_video(const RenderPlan& plan, const std::string& video_path, const std::string& audio_path, std::string& error);

} // namespace tachyon
