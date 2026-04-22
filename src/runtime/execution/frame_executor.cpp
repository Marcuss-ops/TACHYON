#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/core/scene/evaluator.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "frame_executor/frame_executor_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <omp.h>

namespace tachyon {

void FrameExecutor::build_lookup_table(const CompiledScene& scene) {
    m_node_lookup.clear();
    m_property_lookup.clear();
    m_layer_lookup.clear();
    m_composition_lookup.clear();

    for (const auto& composition : scene.compositions) {
        m_node_lookup[composition.node.node_id] = &composition.node;
        m_composition_lookup[composition.node.node_id] = &composition;
        for (const auto& layer : composition.layers) {
            m_node_lookup[layer.node.node_id] = &layer.node;
            m_layer_lookup[layer.node.node_id] = &layer;
        }
    }
    for (const auto& track : scene.property_tracks) {
        m_node_lookup[track.node.node_id] = &track.node;
        m_property_lookup[track.node.node_id] = &track;
    }
}

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context) {
    auto snapshot = std::make_unique<DataSnapshot>();
    snapshot->scene_hash = compiled_scene.scene_hash;
    return execute(compiled_scene, plan, task, *snapshot, context);
}

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context) {

    if (m_node_lookup.empty()) build_lookup_table(compiled_scene);

    ExecutedFrame result;
    result.frame_number = task.frame_number;
    result.scene_hash = compiled_scene.scene_hash;

    const char* diag_env = std::getenv("TACHYON_DIAGNOSTICS");
    const bool diagnostics_enabled = (diag_env && std::string_view(diag_env) == "1");
    if (diagnostics_enabled) context.diagnostic_tracker = &result.diagnostics;

    context.policy = plan.quality_policy;
    context.renderer2d.policy = plan.quality_policy;

    CacheKeyBuilder comp_builder;
    comp_builder.enable_manifest(diagnostics_enabled);
    comp_builder.add_u64(compiled_scene.scene_hash);
    comp_builder.add_u32(compiled_scene.header.layout_version);
    comp_builder.add_u32(compiled_scene.header.compiler_version);
    comp_builder.add_string(plan.quality_tier);
    comp_builder.add_string(plan.compositing_alpha_mode);
    const std::uint64_t composition_key = comp_builder.finish();
    if (diagnostics_enabled) result.diagnostics.composition_key_manifest = comp_builder.manifest();

    CacheKeyBuilder frame_builder = comp_builder;
    frame_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
    const std::uint64_t frame_key = frame_builder.finish();
    result.cache_key = frame_key;
    if (diagnostics_enabled) result.diagnostics.frame_key_manifest = frame_builder.manifest();

    const double fps = compiled_scene.compositions.empty() ? 60.0 : std::max(1u, compiled_scene.compositions.front().fps);
    const double frame_time_seconds = static_cast<double>(task.frame_number) / fps;

    std::uint64_t root_key = 0;
    bool was_cached = false;
    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        root_key = build_node_key(frame_key, root_comp.node);
        was_cached = (m_cache.lookup_composition(root_key) != nullptr);
    }

    if (!was_cached) {
        const auto& topo_order = compiled_scene.graph.topo_order();
        for (std::uint32_t node_id : topo_order) {
            ::tachyon::evaluate_node(*this, node_id, compiled_scene, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, task);
        }
        if (!compiled_scene.compositions.empty()) {
            if (m_cache.lookup_composition(root_key) == nullptr) {
                const CompiledComposition& root_comp = compiled_scene.compositions.front();
                ::tachyon::evaluate_composition(*this, compiled_scene, root_comp, plan, snapshot, context, composition_key, root_key, frame_key, frame_time_seconds, task);
            }
        }
    }

    if (!compiled_scene.compositions.empty()) {
        auto cached_comp = m_cache.lookup_composition(root_key);
        if (cached_comp) {
            result.cache_hit = was_cached;

            if (plan.motion_blur_enabled && plan.motion_blur_samples > 1) {
                int samples = std::min<int>(static_cast<int>(plan.motion_blur_samples), plan.quality_policy.motion_blur_sample_cap);
                const double shutter_duration = (plan.motion_blur_shutter_angle / 360.0) / fps;
                const double shutter_start_offset = (plan.motion_blur_shutter_phase / 360.0) / fps;

                std::vector<std::shared_ptr<renderer2d::SurfaceRGBA>> samples_surfaces(samples);
                #pragma omp parallel for
                for (int s = 0; s < samples; ++s) {
                    const double t_offset = shutter_start_offset + (samples > 1 ? (static_cast<double>(s) / (samples - 1)) * shutter_duration : 0.0);
                    const double sub_frame_time = frame_time_seconds + t_offset;
                    CacheKeyBuilder sample_builder = comp_builder;
                    sample_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
                    sample_builder.add_f64(sub_frame_time);
                    const std::uint64_t sample_key = sample_builder.finish();
                    const std::uint64_t root_sample_key = build_node_key(sample_key, compiled_scene.compositions.front().node);

                    RenderContext thread_context = context;
                    thread_context.renderer2d.accumulation_buffer.resize(0);
                    thread_context.ray_tracer = std::make_shared<renderer3d::RayTracer>();
                    thread_context.renderer2d.ray_tracer = thread_context.ray_tracer;

                    const auto& topo_order = compiled_scene.graph.topo_order();
                    for (std::uint32_t node_id : topo_order) {
                        ::tachyon::evaluate_node(*this, node_id, compiled_scene, plan, snapshot, thread_context, composition_key, sample_key, sub_frame_time, task, frame_key, frame_time_seconds);
                    }
                    if (m_cache.lookup_composition(root_sample_key) == nullptr) {
                         ::tachyon::evaluate_composition(*this, compiled_scene, compiled_scene.compositions.front(), plan, snapshot, thread_context, composition_key, root_sample_key, sample_key, sub_frame_time, task);
                    }
                    auto sub_comp = m_cache.lookup_composition(root_sample_key);
                    if (sub_comp) {
                        RasterizedFrame2D rasterized = render_evaluated_composition_2d(*sub_comp, plan, task, thread_context.renderer2d);
                        if (rasterized.surface) samples_surfaces[s] = rasterized.surface;
                    }
                }

                std::unique_ptr<renderer2d::Framebuffer> accumulated;
                for (int s = 0; s < samples; ++s) {
                    if (samples_surfaces[s]) {
                        if (!accumulated) accumulated = std::make_unique<renderer2d::Framebuffer>(std::move(*samples_surfaces[s]));
                        else {
                            auto& acc_pixels = accumulated->mutable_pixels();
                            const auto& sub_pixels = samples_surfaces[s]->pixels();
                            const std::size_t count = std::min(acc_pixels.size(), sub_pixels.size());
                            for (std::size_t i = 0; i < count; ++i) acc_pixels[i] += sub_pixels[i];
                        }
                    }
                }
                if (accumulated) {
                    auto& acc_pixels = accumulated->mutable_pixels();
                    const float inv_samples = 1.0f / static_cast<float>(samples);
                    for (float& p : acc_pixels) p *= inv_samples;
                    result.frame = std::shared_ptr<renderer2d::Framebuffer>(std::move(accumulated));
                }
            } else {
                renderer2d::DrawListBuilder builder;
                const renderer2d::DrawList2D draw_list = builder.build(*cached_comp);
                result.draw_command_count = draw_list.commands.size();
                RasterizedFrame2D rasterized = render_evaluated_composition_2d(*cached_comp, plan, task, context.renderer2d);
                if (rasterized.surface) result.frame = std::make_shared<renderer2d::Framebuffer>(std::move(*rasterized.surface));
            }
        }
    }

    if (!result.frame || result.frame->width() == 0U || result.frame->height() == 0U) {
        result.frame = std::make_shared<renderer2d::Framebuffer>(
            static_cast<std::uint32_t>(std::max<std::int64_t>(1, plan.composition.width)),
            static_cast<std::uint32_t>(std::max<std::int64_t>(1, plan.composition.height)));
    }
    context.diagnostic_tracker = nullptr;
    return result;
}

// Delegate methods for backward compatibility
void FrameExecutor::evaluate_node(std::uint32_t node_id, const CompiledScene& scene, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t composition_key, std::uint64_t frame_key, double frame_time_seconds, const FrameRenderTask& task, std::optional<std::uint64_t> main_frame_key, std::optional<double> main_frame_time) {
    ::tachyon::evaluate_node(*this, node_id, scene, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, task, main_frame_key, main_frame_time);
}
void FrameExecutor::evaluate_property(const CompiledScene& scene, const CompiledPropertyTrack& track, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t node_key, double frame_time_seconds) {
    ::tachyon::evaluate_property(*this, scene, track, plan, snapshot, context, node_key, frame_time_seconds);
}
void FrameExecutor::evaluate_layer(const CompiledScene& scene, const CompiledLayer& layer, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t composition_key, std::uint64_t frame_key, double frame_time_seconds, std::optional<std::uint64_t> main_frame_key, std::optional<double> main_frame_time) {
    ::tachyon::evaluate_layer(*this, scene, layer, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, main_frame_key, main_frame_time);
}
void FrameExecutor::evaluate_composition(const CompiledScene& scene, const CompiledComposition& comp, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t composition_key, std::uint64_t node_key, std::uint64_t frame_key, double frame_time_seconds, const FrameRenderTask& task) {
    ::tachyon::evaluate_composition(*this, scene, comp, plan, snapshot, context, composition_key, node_key, frame_key, frame_time_seconds, task);
}

ExecutedFrame execute_frame_task(const SceneSpec& scene, const CompiledScene& compiled_scene, const RenderPlan& plan, const FrameRenderTask& task, FrameCache& cache, RenderContext& context) {
    (void)scene;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    auto snapshot = std::make_unique<DataSnapshot>();
    snapshot->scene_hash = compiled_scene.scene_hash;
    return executor.execute(compiled_scene, plan, task, *snapshot, context);
}

EvaluatedFrameState evaluate_frame_state(const SceneSpec& scene, const CompiledScene& compiled_scene, const RenderPlan& plan, const FrameRenderTask& task) {
    EvaluatedFrameState state;
    state.task = task;
    state.scene_hash = compiled_scene.scene_hash;
    if (!scene.compositions.empty()) {
        const auto evaluated = scene::evaluate_scene_composition_state(scene, plan.composition_target, task.frame_number);
        if (evaluated.has_value()) state.composition_state = std::move(*evaluated);
    }
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    renderer2d::DrawListBuilder builder;
    return builder.build(state.composition_state);
}

} // namespace tachyon
