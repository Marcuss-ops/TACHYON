#include "tachyon/runtime/execution/rasterization_step.h"
#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/runtime/profiling/render_profiler.h"

#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#endif

namespace tachyon {

RasterizationResult RasterizationStep::execute(
    const scene::EvaluatedCompositionState& cached_comp,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    runtime::RuntimeSurfacePool* pool,
    profiling::RenderProfiler* profiler,
    std::uint64_t frame_number
) {
    RasterizationResult result;

    renderer2d::DrawListBuilder builder;
    const renderer2d::DrawList2D draw_list = builder.build(cached_comp);
    result.draw_command_count = draw_list.commands.size();

#ifdef TACHYON_ENABLE_3D
    // Inject ray tracer if scene has 3D layers and none has been injected yet
    if (!context.renderer2d.ray_tracer) {
        if (context.renderer2d.modifier_registry) {
            context.renderer2d.ray_tracer = std::make_shared<renderer3d::RayTracer>(
                context.renderer2d.media_manager,
                context.renderer2d.modifier_registry);
        } else {
            // Fallback for safety, though RenderSession should have initialized this
            static renderer3d::Modifier3DRegistry global_fallback;
            static bool initialized = false;
            if (!initialized) {
                ::tachyon::renderer3d::register_builtin_modifiers(global_fallback);
                initialized = true;
            }
            context.renderer2d.ray_tracer = std::make_shared<renderer3d::RayTracer>(
                context.renderer2d.media_manager,
                &global_fallback);
        }
    }
#endif

    RasterizedFrame2D rasterized;
    {
        profiling::ProfileScope raster_scope(profiler, profiling::ProfileEventType::Phase, "composition_raster", frame_number);
        rasterized = render_evaluated_composition_2d(cached_comp, intent, plan, task, context.renderer2d);
    }

    if (rasterized.surface) {
        if (pool) {
            auto pooled = pool->acquire();
            if (pooled) {
                pooled->blit(*rasterized.surface, 0, 0);
                result.frame = std::shared_ptr<renderer2d::SurfaceRGBA>(pooled.release(), [pool](renderer2d::SurfaceRGBA* s) {
                    pool->release(std::unique_ptr<renderer2d::SurfaceRGBA>(s));
                });
            } else {
                result.frame = std::make_shared<renderer2d::Framebuffer>(std::move(*rasterized.surface));
            }
        } else {
            result.frame = std::make_shared<renderer2d::Framebuffer>(std::move(*rasterized.surface));
        }
    }

    result.aovs = std::move(rasterized.aovs);
    return result;
}

} // namespace tachyon
