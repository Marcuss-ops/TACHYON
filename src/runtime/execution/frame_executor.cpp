#include "tachyon/runtime/execution/frame_blend_renderer.h"
#include "tachyon/runtime/execution/motion_blur_sampler.h"
#include "tachyon/runtime/execution/frame_fallback_policy.h"
#include "tachyon/runtime/execution/rasterization_step.h"
#include "frame_executor/frame_executor_internal.h"
#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/core/ray_tracer.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <omp.h>
#include <thread>

namespace tachyon {

namespace {

std::optional<scene::EvaluatedCompositionState> evaluate_target_composition_state(
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task) {
    for (const auto& comp : scene.compositions) {
        if (comp.id != plan.composition_target) {
            continue;
        }

        double fps = comp.frame_rate.value();
        if (fps <= 0.0) {
            fps = 24.0;
        }

        const double time = static_cast<double>(task.frame_number) / fps;
        return scene::evaluate_composition_internal(
            &scene,
            comp,
            task.frame_number,
            time,
            {},
            nullptr,
            {},
            nullptr,
            std::nullopt,
            std::nullopt);
    }

    return std::nullopt;
}

/**
 * @brief Render a single frame at the given time and return the surface
 */
std::shared_ptr<renderer2d::SurfaceRGBA> render_frame_at_time(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    double render_time,
     const std::uint64_t composition_key,
     const std::uint64_t frame_key) {
    // Build a robust cache key using frame_key, render time, composition, and plan params
    CacheKeyBuilder temp_builder;
    temp_builder.add_u64(frame_key);
    temp_builder.add_f64(render_time);
    temp_builder.add_u64(composition_key);
    if (plan.composition.frame_rate.numerator > 0) {
        temp_builder.add_u64(static_cast<std::uint64_t>(plan.composition.frame_rate.numerator));
    }
    if (task.subframe_index.has_value()) {
        temp_builder.add_u64(*task.subframe_index);
    }
    if (task.motion_blur_sample_index.has_value()) {
        temp_builder.add_u64(*task.motion_blur_sample_index);
    }
    const std::uint64_t temp_key = temp_builder.finish();

    // Create a modified task with the target time
    FrameRenderTask temp_task = task;
    temp_task.time_seconds = render_time;

    // Evaluate the scene at the specified time
    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
        if (context.cancel_flag && context.cancel_flag->load()) break;
        ::tachyon::evaluate_node(executor, node_id, compiled_scene, plan, snapshot, context, composition_key, temp_key, render_time, temp_task);
    }

    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        const std::uint64_t root_key = build_node_key(temp_key, root_comp.node);
        ::tachyon::evaluate_composition(executor, compiled_scene, root_comp, plan, snapshot, context, composition_key, root_key, temp_key, render_time, temp_task);
    }

    // Render the evaluated composition
    auto cached_comp = executor.cache().lookup_composition(build_node_key(temp_key, compiled_scene.compositions.front().node));
    if (cached_comp) {
        RasterizedFrame2D rasterized = render_evaluated_composition_2d(*cached_comp, plan, temp_task, context.renderer2d);
        return rasterized.surface;
    }
    return nullptr;
}

/**
 * @brief Convert renderer2d::SurfaceRGBA to timeline::FrameBuffer
 */
timeline::FrameBuffer surface_to_framebuffer(const renderer2d::SurfaceRGBA& src) {
    timeline::FrameBuffer dst;
    dst.width = src.width();
    dst.height = src.height();
    dst.channels = 4; // RGBA
    dst.data.resize(static_cast<size_t>(dst.width) * dst.height * dst.channels);

    const std::size_t dst_width = static_cast<std::size_t>(dst.width);
    const std::size_t dst_height = static_cast<std::size_t>(dst.height);
    const std::size_t dst_channels = static_cast<std::size_t>(dst.channels);
    for (std::size_t y = 0; y < dst_height; ++y) {
        for (std::size_t x = 0; x < dst_width; ++x) {
            const auto pixel = src.get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            const size_t idx = (y * dst_width + x) * dst_channels;
            dst.data[idx + 0] = static_cast<uint8_t>(pixel.r * 255.0f);
            dst.data[idx + 1] = static_cast<uint8_t>(pixel.g * 255.0f);
            dst.data[idx + 2] = static_cast<uint8_t>(pixel.b * 255.0f);
            dst.data[idx + 3] = static_cast<uint8_t>(pixel.a * 255.0f);
        }
    }
    return dst;
}

/**
 * @brief Convert timeline::FrameBuffer to renderer2d::Framebuffer
 */
std::shared_ptr<renderer2d::Framebuffer> framebuffer_to_framebuffer(const timeline::FrameBuffer& src) {
    auto dst = std::make_shared<renderer2d::Framebuffer>(src.width, src.height);
    const std::size_t src_width = static_cast<std::size_t>(src.width);
    const std::size_t src_height = static_cast<std::size_t>(src.height);
    const std::size_t src_channels = static_cast<std::size_t>(src.channels);
    for (std::size_t y = 0; y < src_height; ++y) {
        for (std::size_t x = 0; x < src_width; ++x) {
            const size_t idx = (y * src_width + x) * src_channels;
            renderer2d::Color c(
                src.data[idx + 0] / 255.0f,
                src.data[idx + 1] / 255.0f,
                src.data[idx + 2] / 255.0f,
                src.data[idx + 3] / 255.0f);
            dst->set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), c);
        }
    }
    return dst;
}

} // namespace tachyon

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

    profiling::ProfileScope frame_scope(context.profiler, profiling::ProfileEventType::Frame, "frame_render", task.frame_number);

    if (m_node_lookup.empty()) build_lookup_table(compiled_scene);

    ExecutedFrame result;
    result.frame_number = task.frame_number;
    result.scene_hash = compiled_scene.scene_hash;

    const char* diag_env = std::getenv("TACHYON_DIAGNOSTICS");
    const bool diagnostics_enabled = (diag_env && std::string_view(diag_env) == "1");
    if (diagnostics_enabled) {
        context.diagnostic_tracker = &result.diagnostics;
        context.renderer2d.diagnostics = &result.diagnostics;
    }

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
    double frame_time_seconds = static_cast<double>(task.frame_number) / fps;

    // Time Remap & Frame Blend setup
    std::optional<timeline::FrameBlendResult> blend_result;
    if (plan.time_remap_curve.has_value()) {
        const float dest_time = static_cast<float>(frame_time_seconds);
        frame_time_seconds = static_cast<double>(timeline::evaluate_source_time(*plan.time_remap_curve, dest_time));

        // Calculate blend parameters for frame blending
        const float frame_duration = static_cast<float>(1.0 / fps);
        blend_result = timeline::evaluate_frame_blend(*plan.time_remap_curve, dest_time, frame_duration);
    }

    // 1. Try Frame Blending
    auto blended_frame = FrameBlendRenderer::try_render_blend(
        *this, compiled_scene, plan, task, snapshot, context,
        blend_result, frame_time_seconds, composition_key, frame_key);
    if (blended_frame.has_value()) {
        result.frame = *blended_frame;
        result.cache_hit = false;
        context.diagnostic_tracker = nullptr;
        context.renderer2d.diagnostics = nullptr;
        return result;
    }

    // 2. Try Motion Blur Sampling
    auto mb_frame = MotionBlurSampler::sample(
        *this, compiled_scene, plan, task, snapshot, context,
        frame_time_seconds, composition_key, frame_key, static_cast<int>(fps), comp_builder);
    if (mb_frame.has_value()) {
        result.frame = *mb_frame;
        result.cache_hit = false;
        context.diagnostic_tracker = nullptr;
        context.renderer2d.diagnostics = nullptr;
        return result;
    }

    // 3. Normal Path: Evaluate Scene
    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
        if (context.cancel_flag && context.cancel_flag->load()) break;
        ::tachyon::evaluate_node(*this, node_id, compiled_scene, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, task);
    }

    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        const std::uint64_t root_key = build_node_key(frame_key, root_comp.node);
        ::tachyon::evaluate_composition(*this, compiled_scene, root_comp, plan, snapshot, context, composition_key, root_key, frame_key, frame_time_seconds, task);

        auto cached_comp = m_cache.lookup_composition(root_key);
        if (cached_comp) {
            // 4. Rasterization Step
            auto raster_result = RasterizationStep::execute(
                *cached_comp, plan, task, context, m_pool, context.profiler, task.frame_number);
            result.frame = raster_result.frame;
            result.aovs = std::move(raster_result.aovs);
            result.draw_command_count = raster_result.draw_command_count;
        }
    }

    // 5. Apply Fallback Policy
    FrameFallbackPolicy::apply(result, compiled_scene, plan);

    context.diagnostic_tracker = nullptr;
    context.renderer2d.diagnostics = nullptr;
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
    if (const auto composition = evaluate_target_composition_state(scene, plan, task); composition.has_value()) {
        state.composition_state = std::move(*composition);
    }
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    renderer2d::DrawListBuilder builder;
    return builder.build(state.composition_state);
}

} // namespace tachyon
