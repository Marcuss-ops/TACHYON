#include "tachyon/runtime/execution/session/frame_execution_loop.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/runtime/profiling/render_profiler.h"

namespace tachyon {

void FrameExecutionLoop::run(
    RenderSessionState& state,
    const CompiledScene& compiled_scene,
    FrameCache& cache,
    const ::tachyon::runtime::RenderWorkerBudget& budget,
    media::IMediaPrefetcher* prefetcher,
    CancelFlag* cancel_flag,
    RenderSessionResult& result
) {
    state.frame_times.resize(state.effective_plan.frame_tasks.size());

    const auto start = std::chrono::high_resolution_clock::now();

    profiling::ProfileScope scope(
        state.context.profiler,
        profiling::ProfileEventType::Phase,
        "render_frames_loop");

    render_frames_parallel_internal(
        compiled_scene,
        state.effective_plan,
        cache,
        budget,
        state.context,
#ifdef TACHYON_ENABLE_MEDIA
        prefetcher,
#else
        nullptr,
#endif
        nullptr,
        state.rendered_frames,
        nullptr,
        cancel_flag,
        nullptr,
        state.sink.get(),
        &result,
        &state.frame_times
    );

    const auto end = std::chrono::high_resolution_clock::now();

    result.frame_execution_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    result.frame_times_ms = std::move(state.frame_times);
    result.frames = std::move(state.rendered_frames);

    result.cache_hits = 0;
    result.cache_misses = 0;

    for (const auto& frame : result.frames) {
        if (frame.cache_hit) {
            ++result.cache_hits;
        } else {
            ++result.cache_misses;
        }
    }
}

} // namespace tachyon
