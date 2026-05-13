#include "tachyon/runtime/execution/rasterization_step.h"
#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/runtime/profiling/render_profiler.h"


namespace tachyon {

RasterizationResult RasterizationStep::execute(
    const scene::EvaluatedCompositionState& cached_comp,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    SurfacePool* pool,
    profiling::RenderProfiler* profiler,
    std::uint64_t frame_number
) {
    RasterizationResult result;

    renderer2d::DrawListBuilder builder;
    const renderer2d::DrawList2D draw_list = builder.build(cached_comp);
    result.draw_command_count = draw_list.commands.size();


    RasterizedFrame2D rasterized;
    {
        profiling::ProfileScope raster_scope(profiler, profiling::ProfileEventType::Phase, "composition_raster", frame_number);
        renderer2d::EffectRegistry effect_reg;
        rasterized = render_evaluated_composition_2d(cached_comp, intent, plan, task, context, effect_reg);
    }

    if (rasterized.surface) {
        if (pool) {
            auto pooled = pool->acquire_prepared();
            if (pooled) {
                pooled->blit(*rasterized.surface, 0, 0);
                result.frame = std::static_pointer_cast<renderer2d::Framebuffer>(pooled);
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
