#pragma once

#include "tachyon/runtime/execution/session/render_session_state.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/profiling/render_profiler.h"

namespace tachyon {

class SinkLifecycle {
public:
    static void create(RenderSessionState& state);

    static bool begin(
        RenderSessionState& state,
        RenderSessionResult& result,
        profiling::RenderProfiler* profiler
    );

    static void finish(
        RenderSessionState& state,
        RenderSessionResult& result
    );
};

} // namespace tachyon
