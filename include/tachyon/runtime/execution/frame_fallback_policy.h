#pragma once

#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <memory>

namespace tachyon {

class FrameFallbackPolicy {
public:
    /**
     * @brief Apply fallback policy if result has no valid frame.
     * Modifies result in-place: sets fallback frame or marks failure.
     */
    static void apply(ExecutedFrame& result, const CompiledScene& scene, const RenderPlan& plan);
};

} // namespace tachyon
