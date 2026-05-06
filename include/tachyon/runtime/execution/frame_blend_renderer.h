#pragma once

#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/timeline/frame_blend.h"

#include <optional>
#include <memory>

namespace tachyon {

class FrameBlendRenderer {
public:
    /**
     * @brief Try to render a blended frame using precomputed blend result.
     * 
     * If blending is not needed (no blend result, or blend factor is extreme),
     * returns std::nullopt. Otherwise, renders frame A and B and blends them.
     */
    static std::optional<std::shared_ptr<renderer2d::Framebuffer>> try_render_blend(
        FrameExecutor& executor,
        const CompiledScene& scene,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        const DataSnapshot& snapshot,
        RenderContext& context,
        const std::optional<timeline::FrameBlendResult>& blend_result,
        double frame_time_seconds,
        std::uint64_t composition_key,
        std::uint64_t frame_key
    );
};

} // namespace tachyon
