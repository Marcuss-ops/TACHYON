#pragma once

#include "tachyon/runtime/execution/session/render_session_state.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/session/render_session.h"

namespace tachyon {

struct RenderSessionPreflightInput {
    const SceneSpec& scene;
    const CompiledScene& compiled_scene;
    const RenderExecutionPlan& execution_plan;
    const RenderContext& context;
};

class RenderSessionPreflight {
public:
    static bool run(
        const RenderSessionPreflightInput& input,
        RenderSessionResult& result
    );
};

} // namespace tachyon
