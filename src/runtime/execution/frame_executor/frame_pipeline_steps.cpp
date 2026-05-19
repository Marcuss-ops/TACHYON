#include "tachyon/runtime/execution/frame_executor/frame_pipeline_steps.h"
#include "tachyon/runtime/execution/bounds/layer_bounds.h"
#include "tachyon/runtime/execution/bounds/invalidation_diagnostics.h"
#include "frame_executor_internal.h"
#include "tachyon/runtime/execution/frame_blend_renderer.h"
#include "tachyon/runtime/execution/motion_blur_sampler.h"
#include "tachyon/runtime/execution/frame_fallback_policy.h"
#include "tachyon/runtime/execution/rasterization_step.h"
#include "tachyon/render/intent_builder.h"
#include "tachyon/renderer2d/resource/resource_provider.h"
#include "tachyon/runtime/execution/session/render_internal.h"

namespace tachyon {

std::optional<ExecutedFrame> try_frame_blend_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    auto blended_frame = FrameBlendRenderer::try_render_blend(
        executor,
        compiled_scene,
        plan,
        task,
        snapshot,
        context,
        timing_state.blend_result,
        timing_state.frame_time_seconds,
        cache_state.composition_key,
        cache_state.frame_key);

    if (blended_frame.has_value()) {
        ExecutedFrame result;
        result.frame_number = task.frame_number;
        result.scene_hash = compiled_scene.scene_hash;
        result.frame = *blended_frame;
        result.cache_hit = false;
        return result;
    }

    return std::nullopt;
}

std::optional<ExecutedFrame> try_motion_blur_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    auto mb_frame = MotionBlurSampler::sample(
        executor,
        compiled_scene,
        plan,
        task,
        snapshot,
        context,
        timing_state.frame_time_seconds,
        cache_state.composition_key,
        cache_state.frame_key,
        static_cast<int>(timing_state.fps),
        cache_state.composition_builder);

    if (mb_frame.has_value()) {
        ExecutedFrame result;
        result.frame_number = task.frame_number;
        result.scene_hash = compiled_scene.scene_hash;
        result.frame = *mb_frame;
        result.cache_hit = false;
        return result;
    }

    return std::nullopt;
}

void evaluate_frame_graph_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {


    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
        if (context.cancel_flag && context.cancel_flag->load()) {
            break;
        }
        ::tachyon::evaluate_node(
            executor,
            node_id,
            compiled_scene,
            plan,
            snapshot,
            context,
            cache_state.composition_key,
            cache_state.frame_key,
            timing_state.frame_time_seconds,
            task);
    }

    // PR2: Calculate DirtyRegion and store in diagnostics
    if (context.diagnostics) {
        if (!context.diagnostics->invalidation) {
            context.diagnostics->invalidation = std::make_unique<FrameInvalidationDiagnostics>();
        }
        auto& inv = *context.diagnostics->invalidation;
        const int comp_width = static_cast<int>(plan.composition.width);
        const int comp_height = static_cast<int>(plan.composition.height);
        const renderer2d::IntRect frame_rect{0, 0, comp_width, comp_height};

        auto& prev_bounds_map = executor.previous_layer_bounds();

        // If it's the first frame, everything is dirty
        if (task.frame_number == plan.frame_range.start) {
            inv.full_frame_invalidation = true;
        }

        // Evaluate bounds for layers in the target composition
        for (const auto& comp : compiled_scene.compositions) {
            if (comp.composition_id != plan.composition_target && 
                !(plan.composition_target.empty() && &comp == &compiled_scene.compositions.front())) {
                continue;
            }

            for (const auto& layer : comp.layers) {
                const auto bounds_res = LayerBoundsEvaluator::evaluate(
                    layer, timing_state.frame_time_seconds, comp_width, comp_height);
                
                LayerFrameBounds frame_bounds;
                frame_bounds.layer_id = layer.node.node_id;
                frame_bounds.current_bounds = bounds_res.world_bounds.clipped_to(frame_rect);
                frame_bounds.full_frame = bounds_res.full_frame;

                auto it = prev_bounds_map.find(layer.node.node_id);
                if (it != prev_bounds_map.end()) {
                    frame_bounds.previous_bounds = it->second;
                } else {
                    frame_bounds.previous_bounds = renderer2d::IntRect{};
                }

                if (frame_bounds.full_frame) {
                    frame_bounds.dirty_bounds = frame_rect;
                    inv.full_frame_invalidation = true;
                } else {
                    frame_bounds.dirty_bounds = frame_bounds.current_bounds.united(frame_bounds.previous_bounds).clipped_to(frame_rect);
                }

                if (!frame_bounds.dirty_bounds.empty()) {
                    inv.dirty_region.add(frame_bounds.dirty_bounds);
                }

                inv.layer_bounds.push_back(frame_bounds);
                
                // Update persistent state for next frame
                prev_bounds_map[layer.node.node_id] = frame_bounds.current_bounds;
            }
        }
        
        if (inv.full_frame_invalidation) {
            inv.dirty_region.clear();
            inv.dirty_region.add(frame_rect);
        }
        
        inv.dirty_region.optimize();
    }
}

void evaluate_and_rasterize_root_composition_step(
    ExecutedFrame& result,
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition* root_comp_ptr = nullptr;
        if (!plan.composition_target.empty()) {
            for (const auto& comp : compiled_scene.compositions) {
                if (comp.composition_id == plan.composition_target) {
                    root_comp_ptr = &comp;
                    break;
                }
            }
        }
        if (!root_comp_ptr) {
            root_comp_ptr = &compiled_scene.compositions.front();
        }

        const CompiledComposition& root_comp = *root_comp_ptr;
        const std::uint64_t root_key = build_node_key(cache_state.frame_key, root_comp.node);
        ::tachyon::evaluate_composition(
            executor,
            compiled_scene,
            root_comp,
            plan,
            snapshot,
            context,
            cache_state.composition_key,
            root_key,
            cache_state.frame_key,
            timing_state.frame_time_seconds,
            task);



        auto cached_comp = executor.cache().lookup_composition(root_key);
        if (cached_comp) {
            renderer2d::RendererResourceProvider provider(context);
            const auto intent_result = render::build_render_intent(*cached_comp, &provider);

            if (context.diagnostics) {
                for (const auto& diag : intent_result.diagnostics.diagnostics) {
                    if (diag.severity == DiagnosticSeverity::Error) {
                        context.diagnostics->diagnostics.add_error(
                            "render.intent.build_error",
                            diag.message,
                            diag.path);
                    } else if (diag.severity == DiagnosticSeverity::Warning) {
                        context.diagnostics->diagnostics.add_warning(
                            "render.intent.build_warning",
                            diag.message,
                            diag.path);
                    }
                }
            }

            RasterizationResult raster_result = RasterizationStep::execute(
                *cached_comp,
                intent_result.intent,
                plan,
                task,
                context,
                executor.surface_pool(),
                context.profiler,
                task.frame_number);
            result.frame = raster_result.frame;
            result.aovs = std::move(raster_result.aovs);
            result.draw_command_count = raster_result.draw_command_count;
        }
    }
}

void finalize_frame_step(
    ExecutedFrame& result,
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const FrameCacheState& cache_state) {

    FrameFallbackPolicy::apply(result, compiled_scene, plan);

    if (task.cacheable && result.frame) {
        executor.cache().store_frame(cache_state.frame_key, result.frame);
    }

    context.diagnostics = nullptr;
    context.diagnostics = nullptr;
}

} // namespace tachyon
