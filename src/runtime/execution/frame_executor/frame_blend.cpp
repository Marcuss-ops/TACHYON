#include "tachyon/runtime/execution/frame_blend_renderer.h"
#include "frame_executor_internal.h"
#include "tachyon/timeline/time_remap.h"
#include "tachyon/timeline/frame_blend.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/profiling/render_profiler.h"

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
    double frame_time_seconds,
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
            if (plan.frame_blend_mode == timeline::FrameBlendMode::PixelMotion ||
                plan.frame_blend_mode == timeline::FrameBlendMode::OpticalFlow) {
                timeline::FrameBuffer fb_a = surface_to_framebuffer(*frame_a_surface);
                timeline::FrameBuffer fb_b = surface_to_framebuffer(*frame_b_surface);
                
                timeline::DefaultOpticalFlowProvider flow_provider;
                timeline::OpticalFlowField flow_ab = flow_provider.compute_optical_flow(fb_a, fb_b);
                timeline::OpticalFlowField flow_ba = flow_provider.compute_optical_flow(fb_b, fb_a);
                
                timeline::FrameBuffer blended = flow_provider.synthesize_frame_optical_flow(fb_a, fb_b, flow_ab, flow_ba, blend_result->blend_factor);
                return framebuffer_to_framebuffer(blended);
            } else {
                auto blended_surface = std::make_shared<renderer2d::SurfaceRGBA>(
                    frame_a_surface->width(), frame_a_surface->height());
                const float blend = blend_result->blend_factor;
                const std::size_t width = static_cast<std::size_t>(blended_surface->width());
                const std::size_t height = static_cast<std::size_t>(blended_surface->height());
                for (std::size_t y = 0; y < height; ++y) {
                    for (std::size_t x = 0; x < width; ++x) {
                        auto a = frame_a_surface->get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
                        auto b = frame_b_surface->get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
                        renderer2d::Color c(
                            a.r * (1.0f - blend) + b.r * blend,
                            a.g * (1.0f - blend) + b.g * blend,
                            a.b * (1.0f - blend) + b.b * blend,
                            a.a * (1.0f - blend) + b.a * blend
                        );
                        blended_surface->set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), c);
                    }
                }
                return std::make_shared<renderer2d::Framebuffer>(*blended_surface);
            }
        }
    }
    return std::nullopt;
}

} // namespace tachyon
