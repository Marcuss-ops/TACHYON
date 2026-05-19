#include "tachyon/runtime/execution/session/sink_lifecycle.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/output/output_planner.h"
#include "tachyon/output/output_utils.h"

namespace tachyon {

void SinkLifecycle::create(RenderSessionState& state) {
    state.sink = output::create_frame_output_sink(state.effective_plan.render_plan);
    
    const bool is_video = output::output_requests_video_file(state.effective_plan.render_plan.output);
    state.audio_plan = output::plan_audio_export(state.effective_plan.render_plan, is_video);
    
    if (state.sink && !state.audio_plan.path.empty() && state.audio_plan.is_temporary) {
        state.sink->set_audio_source(state.audio_plan.path.string());
    }
}

bool SinkLifecycle::begin(
    RenderSessionState& state,
    RenderSessionResult& result,
    profiling::RenderProfiler* profiler
) {
    if (!state.sink) {
        return true;
    }

    state.sink->set_profiler(profiler);
    profiling::ProfileScope scope(profiler, profiling::ProfileEventType::Phase, "initialize_sink");
    if (state.sink->begin(state.effective_plan.render_plan)) {
        result.output_configured = true;
        return true;
    }

    result.output_error = state.sink->last_error();
    return false;
}

void SinkLifecycle::finish(
    RenderSessionState& state,
    RenderSessionResult& result
) {
    if (state.sink) {
        profiling::ProfileScope scope(
            state.context.profiler,
            profiling::ProfileEventType::Phase,
            "finalize_sink");
        const auto encode_start = std::chrono::high_resolution_clock::now();
        if (result.output_error.empty() && !state.sink->finish()) {
            result.output_error = state.sink->last_error();
        }
        const auto encode_end = std::chrono::high_resolution_clock::now();
        result.encode_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();
    }
}

} // namespace tachyon
