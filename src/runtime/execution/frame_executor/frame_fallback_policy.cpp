#include "tachyon/runtime/execution/frame_fallback_policy.h"

namespace tachyon {

void FrameFallbackPolicy::apply(ExecutedFrame& result, const CompiledScene& scene, const RenderPlan& plan) {
    if (!result.frame || result.frame->width() == 0U || result.frame->height() == 0U) {
        if (scene.compositions.empty()) {
            // Intentionally empty scene: generate explicit fallback frame
            result.frame = std::make_shared<renderer2d::Framebuffer>(
                static_cast<std::uint32_t>(std::max<std::int64_t>(1, plan.composition.width)),
                static_cast<std::uint32_t>(std::max<std::int64_t>(1, plan.composition.height)));
            result.error = "Empty scene: using fallback frame";
        } else {
            // Rendering failed: no valid frame produced
            result.success = false;
            result.error = "Frame rendering failed: no valid framebuffer generated";
        }
    }
}

} // namespace tachyon
