#include "tachyon/runtime/execution/session/render_session_state.h"

namespace tachyon {

std::string RenderSessionPlanResolver::resolve_output_path(
    const std::filesystem::path& output_path,
    const RenderPlan& plan
) {
    if (!output_path.empty()) {
        return output_path.string();
    }
    return plan.output.destination.path;
}

void RenderSessionPlanResolver::apply_output_override(
    RenderSessionState& state,
    const std::filesystem::path& output_path
) {
    if (!state.resolved_output_path.empty()) {
        state.effective_plan.render_plan.output.destination.path = state.resolved_output_path;
    }
}

} // namespace tachyon
