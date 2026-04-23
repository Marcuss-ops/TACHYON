#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/timeline/time_remap.h"
#include "tachyon/timeline/frame_blend.h"
#include "frame_executor/frame_executor_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <omp.h>

namespace tachyon {

namespace {

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
    (void)frame_key;

    // Create a modified task with the target time
    FrameRenderTask temp_task = task;
    temp_task.time_seconds = render_time;

    // Build a new frame key for this time
    CacheKeyBuilder temp_builder;
    temp_builder.add_u64(static_cast<std::uint64_t>(render_time * 1000.0)); // Use time as key component
    const std::uint64_t temp_key = temp_builder.finish();

    // Evaluate the scene at the specified time
    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
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

std::shared_ptr<renderer2d::SurfaceRGBA> blend_surfaces_linear(
    const renderer2d::SurfaceRGBA& a,
    const renderer2d::SurfaceRGBA& b,
    float factor) {
    
    if (a.width() != b.width() || a.height() != b.height()) {
        return std::make_shared<renderer2d::SurfaceRGBA>(a);
    }
    
    auto result = std::make_shared<renderer2d::SurfaceRGBA>(a.width(), a.height());
    const auto& pixels_a = a.pixels();
    const auto& pixels_b = b.pixels();
    auto& pixels_res = result->mutable_pixels();
    
    // factor 0.0 = A, 1.0 = B
    const float fa = 1.0f - factor;
    const float fb = factor;
    
    for (size_t i = 0; i < pixels_res.size(); ++i) {
        pixels_res[i] = pixels_a[i] * fa + pixels_b[i] * fb;
    }
    
    return result;
}

} // anonymous namespace

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

    // Frame Blend early out: if blending, render A and B and return early.
    // We don't evaluate the scene at `frame_time_seconds` if we are just going to blend.
    if (blend_result.has_value() && blend_result->blend_factor > 1e-6f && blend_result->blend_factor < 1.0f - 1e-6f) {
        // Render frame A at source_time_a
        auto frame_a_surface = render_frame_at_time(
            *this, compiled_scene, plan, task, snapshot, context,
            blend_result->source_time_a, composition_key, frame_key);

        // Render frame B at source_time_b
        auto frame_b_surface = render_frame_at_time(
            *this, compiled_scene, plan, task, snapshot, context,
            blend_result->source_time_b, composition_key, frame_key);

        if (frame_a_surface && frame_b_surface) {
            // Convert to timeline FrameBuffer for blending
            timeline::FrameBuffer fb_a = surface_to_framebuffer(*frame_a_surface);
            timeline::FrameBuffer fb_b = surface_to_framebuffer(*frame_b_surface);

            // Apply blend based on mode
            timeline::FrameBuffer blended;
            if (plan.frame_blend_mode == timeline::FrameBlendMode::PixelMotion ||
                plan.frame_blend_mode == timeline::FrameBlendMode::OpticalFlow) {
                
                // Use optical flow provider abstraction
                timeline::DefaultOpticalFlowProvider flow_provider;
                
                // Compute bi-directional optical flow
                timeline::OpticalFlowField flow_ab = flow_provider.compute_optical_flow(fb_a, fb_b);
                timeline::OpticalFlowField flow_ba = flow_provider.compute_optical_flow(fb_b, fb_a);
                
                blended = flow_provider.synthesize_frame_optical_flow(fb_a, fb_b, flow_ab, flow_ba, blend_result->blend_factor);
            } else {
                blended = timeline::blend_linear(fb_a, fb_b, blend_result->blend_factor);
            }

            // Convert back to Framebuffer and return
            result.frame = framebuffer_to_framebuffer(blended);
            context.diagnostic_tracker = nullptr;
            return result;
        }
    }

    std::uint64_t root_key = 0;
    bool was_cached = false;
    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        root_key = build_node_key(frame_key, root_comp.node);
        was_cached = (cache().lookup_composition(root_key) != nullptr);
    }

    if (!was_cached) {
        const auto& topo_order = compiled_scene.graph.topo_order();
        for (std::uint32_t node_id : topo_order) {
            ::tachyon::evaluate_node(*this, node_id, compiled_scene, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, task);
        }
        if (!compiled_scene.compositions.empty()) {
            if (cache().lookup_composition(root_key) == nullptr) {
                const CompiledComposition& root_comp = compiled_scene.compositions.front();
                ::tachyon::evaluate_composition(*this, compiled_scene, root_comp, plan, snapshot, context, composition_key, root_key, frame_key, frame_time_seconds, task);
            }
        }
    }

    if (!compiled_scene.compositions.empty()) {
        auto cached_comp = cache().lookup_composition(root_key);
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
                    if (cache().lookup_composition(root_sample_key) == nullptr) {
                         ::tachyon::evaluate_composition(*this, compiled_scene, compiled_scene.compositions.front(), plan, snapshot, thread_context, composition_key, root_sample_key, sample_key, sub_frame_time, task);
                    }
                    auto sub_comp = cache().lookup_composition(root_sample_key);
                    if (sub_comp) {
                        RasterizedFrame2D rasterized = render_evaluated_composition_2d(*sub_comp, plan, task, thread_context.renderer2d);
                        if (rasterized.surface) samples_surfaces[s] = rasterized.surface;
                        // For motion blur samples, we might not want all AOVs per sample, 
                        // but beauty is mandatory.
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
                if (rasterized.surface) {
                    if (m_pool) {
                        auto pooled = m_pool->acquire();
                        if (pooled) {
                            pooled->blit(*rasterized.surface, 0, 0);
                            result.frame = std::shared_ptr<renderer2d::SurfaceRGBA>(pooled.release(), [pool = m_pool](renderer2d::SurfaceRGBA* s) {
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
