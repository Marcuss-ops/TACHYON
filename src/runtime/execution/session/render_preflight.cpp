#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/session/render_session_preflight.h"

namespace tachyon {

bool RenderSession::preflight(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const RenderContext& context,
    RenderSessionResult& result) {
    
    RenderSessionPreflightInput preflight_input{
        scene,
        compiled_scene,
        execution_plan,
        context
    };

    return RenderSessionPreflight::run(preflight_input, result);
}

} // namespace tachyon

