#pragma once

#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <optional>
#include <memory>
#include <vector>

namespace tachyon {

class MotionBlurSampler {
public:
    /**
     * @brief Sample multiple sub-frames for motion blur and accumulate into a single frame.
     * Returns nullopt if motion blur is disabled or not applicable.
     */
    static std::optional<std::shared_ptr<renderer2d::Framebuffer>> sample(
        FrameExecutor& executor,
        const CompiledScene& scene,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        const DataSnapshot& snapshot,
        RenderContext& context,
        double frame_time_seconds,
        std::uint64_t composition_key,
        std::uint64_t frame_key,
        int fps,
        const CacheKeyBuilder& comp_builder
    );
};

} // namespace tachyon
