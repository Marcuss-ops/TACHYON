#include "tachyon/runtime/execution/frame_blend_renderer.h"
#include "frame_executor_internal.h"
#include "tachyon/timeline/time_remap.h"
#include "tachyon/timeline/frame_blend.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/core/transition/transition_simd_kernels.h"

#include <optional>
#include <memory>
#include <cmath>

namespace tachyon {

std::optional<std::shared_ptr<renderer2d::Framebuffer>> FrameBlendRenderer::try_render_blend(
    FrameExecutor& executor,
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const std::optional<timeline::FrameBlendResult>& blend_result,
    [[maybe_unused]] double frame_time_seconds,
    std::uint64_t composition_key,
    std::uint64_t frame_key
) {
    if (blend_result.has_value() && blend_result->blend_factor > 1e-6f && blend_result->blend_factor < 1.0f - 1e-6f) {
        profiling::ProfileScope blend_scope(context.profiler, profiling::ProfileEventType::Phase, "frame_blend", task.frame_number);

        auto frame_a_surface = render_frame_at_time(
            executor, scene, plan, task, snapshot, context,
            blend_result->source_time_a, composition_key, frame_key);

        auto frame_b_surface = render_frame_at_time(
            executor, scene, plan, task, snapshot, context,
            blend_result->source_time_b, composition_key, frame_key);

        if (frame_a_surface && frame_b_surface) {
            // Invariant: Both src (a, b) and dst (out) buffers must be 32-byte aligned for safe SIMD Highway bulk kernel processing.
            // Linear blend only for now
            auto blended_surface = std::make_shared<renderer2d::SurfaceRGBA>(
                frame_a_surface->width(), frame_a_surface->height());
            const float blend = blend_result->blend_factor;
            const std::size_t width = static_cast<std::size_t>(blended_surface->width());
            const std::size_t height = static_cast<std::size_t>(blended_surface->height());
            
            const float* a = frame_a_surface->data();
            const float* b = frame_b_surface->data();
            float* out = blended_surface->data();

            runtime::simd::lerp_pixels_best(out, a, b, width * height * 4, blend);

            return std::make_shared<renderer2d::Framebuffer>(*blended_surface);
        }
    }
    return std::nullopt;
}

} // namespace tachyon
