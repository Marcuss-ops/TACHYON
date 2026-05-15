#include "tachyon/runtime/transitions/transition_ops.h"
#include "tachyon/core/transition/transition_resolver.h"
#include <algorithm>

namespace tachyon::runtime::transitions {

core::MediaResult<renderer2d::SurfaceRGBA>
TransitionOps::render(const TransitionRenderRequest& request, const TransitionRegistry& registry) {
    if (request.transition_id.empty()) {
        return core::MediaResult<renderer2d::SurfaceRGBA>::failure(
            core::MediaError(core::MediaErrorCode::Timeline, "Transition ID is empty")
        );
    }
    
    if (!request.from) {
        return core::MediaResult<renderer2d::SurfaceRGBA>::failure(
            core::MediaError(core::MediaErrorCode::Timeline, "Source surface 'from' is null")
        );
    }

    float progress = std::clamp(request.progress, 0.0f, 1.0f);

    auto resolved = resolve_transition_by_id(request.transition_id, registry);
    if (!resolved.valid) {
        return core::MediaResult<renderer2d::SurfaceRGBA>::failure(
            core::MediaError(core::MediaErrorCode::Timeline, resolved.error_message)
        );
    }

    // Create output surface as a copy of 'from'
    renderer2d::SurfaceRGBA output = *request.from;

    if (resolved.direct_cpu_function) {
        resolved.direct_cpu_function(
            output,
            *request.from,
            request.to,
            progress,
            request.thread_count
        );

        return core::MediaResult<renderer2d::SurfaceRGBA>::success(std::move(output));
    }

    return core::MediaResult<renderer2d::SurfaceRGBA>::failure(
        core::MediaError(core::MediaErrorCode::Effects, "Transition has no direct CPU implementation")
    );
}

core::MediaResult<renderer2d::SurfaceRGBA>
TransitionOps::render_builtin(const TransitionRenderRequest& request) {
    auto registry = create_default_transition_registry();
    return render(request, registry);
}

core::MediaResult<renderer2d::SurfaceRGBA>
apply_transition(
    const renderer2d::SurfaceRGBA& from,
    const renderer2d::SurfaceRGBA* to,
    std::string_view id,
    float progress
) {
    TransitionRenderRequest req;
    req.transition_id = std::string(id);
    req.from = &from;
    req.to = to;
    req.progress = progress;
    req.thread_count = 1;
    
    return TransitionOps::render_builtin(req);
}

} // namespace tachyon::runtime::transitions
