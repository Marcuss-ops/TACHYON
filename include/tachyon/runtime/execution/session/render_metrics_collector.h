#pragma once

#include "tachyon/runtime/execution/session/render_session_state.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/session/scoped_process_sampler.h"

namespace tachyon {

class RenderMetricsCollector {
public:
    static void finalize_session(
        RenderSessionState& state,
        RenderSessionResult& result,
        ScopedProcessSampler& sampler
    );

    static void log_frame_events(
        const RenderSessionState& state,
        const RenderSessionResult& result
    );

    static void persist_telemetry(
        const RenderSessionState& state,
        const RenderSessionResult& result
    );
};

} // namespace tachyon
