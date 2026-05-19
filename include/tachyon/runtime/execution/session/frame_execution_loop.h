#pragma once

#include "tachyon/runtime/execution/session/render_session_state.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/policy/worker_budget.h"
#include "tachyon/core/media/media_interfaces.h"

namespace tachyon {

class FrameExecutionLoop {
public:
    static void run(
        RenderSessionState& state,
        const CompiledScene& compiled_scene,
        FrameCache& cache,
        const ::tachyon::runtime::RenderWorkerBudget& budget,
        media::IMediaPrefetcher* prefetcher,
        CancelFlag* cancel_flag,
        RenderSessionResult& result
    );
};

} // namespace tachyon
