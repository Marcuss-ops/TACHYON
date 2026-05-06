#pragma once

#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"

#include <memory>
#include <vector>

namespace tachyon {

struct RasterizationResult {
    std::shared_ptr<renderer2d::Framebuffer> frame;
    std::vector<output::FrameAOV> aovs;
    std::size_t draw_command_count{0};
};

class RasterizationStep {
public:
    /**
     * @brief Rasterize an evaluated composition into a framebuffer.
     * Handles 3D injection, pooling, and profiling.
     */
    static RasterizationResult execute(
        const scene::EvaluatedCompositionState& cached_comp,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        RenderContext& context,
        runtime::RuntimeSurfacePool* pool,
        profiling::RenderProfiler* profiler,
        std::uint64_t frame_number
    );
};

} // namespace tachyon
