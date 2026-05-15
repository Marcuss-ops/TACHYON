#include "tachyon/ops/transition_ops.h"
#include "tachyon/backends/backend_registry.h"
#include "tachyon/runtime/transitions/transition_ops.h"

namespace tachyon::ops {

core::MediaResult<renderer2d::SurfaceRGBA> TransitionOps::apply(
    const renderer2d::SurfaceRGBA& from,
    const renderer2d::SurfaceRGBA* to,
    const std::string& transition_id,
    float progress
) {
    auto& reg = backends::BackendRegistry::instance();
    auto renderer = reg.create_transition_renderer();
    
    if (renderer) {
        return renderer->render(from, to, progress);
    }
    
    // Fallback to runtime builtin transitions if no backend is registered
    return runtime::transitions::apply_transition(from, to, transition_id, progress);
}

} // namespace tachyon::ops
