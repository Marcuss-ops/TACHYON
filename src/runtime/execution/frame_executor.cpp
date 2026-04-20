#include "tachyon/runtime/execution/frame_executor.h"

#include "tachyon/renderer2d/evaluated_composition_renderer.h"
#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/draw_list_rasterizer.h"
#include "tachyon/renderer2d/texture_resolver.h"
#include "tachyon/core/scene/evaluator.h"
#include "tachyon/renderer3d/ray_tracer.h"

#include <sstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <filesystem>

namespace tachyon {
namespace {

std::string build_summary(const scene::EvaluatedCompositionState& state) {
    std::ostringstream stream;
    stream << state.composition_name << ' ' << state.width << 'x' << state.height << " frame=" << state.frame_number
           << " layers=" << state.layers.size();
    return stream.str();
}

renderer2d::Framebuffer resize_frame_bilinear(
    const renderer2d::Framebuffer& input,
    std::uint32_t target_width,
    std::uint32_t target_height) {

    if (input.width() == target_width && input.height() == target_height) {
        return input;
    }

    renderer2d::Framebuffer output(target_width, target_height);
    if (target_width == 0 || target_height == 0 || input.width() == 0 || input.height() == 0) {
        return output;
    }

    const float x_scale = target_width > 1
        ? static_cast<float>(input.width() - 1U) / static_cast<float>(target_width - 1U)
        : 0.0f;
    const float y_scale = target_height > 1
        ? static_cast<float>(input.height() - 1U) / static_cast<float>(target_height - 1U)
        : 0.0f;

    for (std::uint32_t y = 0; y < target_height; ++y) {
        const float src_y = static_cast<float>(y) * y_scale;
        const std::uint32_t y0 = static_cast<std::uint32_t>(src_y);
        const std::uint32_t y1 = std::min(y0 + 1U, input.height() - 1U);
        const float fy = src_y - static_cast<float>(y0);

        for (std::uint32_t x = 0; x < target_width; ++x) {
            const float src_x = static_cast<float>(x) * x_scale;
            const std::uint32_t x0 = static_cast<std::uint32_t>(src_x);
            const std::uint32_t x1 = std::min(x0 + 1U, input.width() - 1U);
            const float fx = src_x - static_cast<float>(x0);

            const auto c00 = input.get_pixel(x0, y0);
            const auto c10 = input.get_pixel(x1, y0);
            const auto c01 = input.get_pixel(x0, y1);
            const auto c11 = input.get_pixel(x1, y1);

            const auto lerp = [](float a, float b, float t) {
                return a + (b - a) * t;
            };

            const float r0 = lerp(static_cast<float>(c00.r), static_cast<float>(c10.r), fx);
            const float g0 = lerp(static_cast<float>(c00.g), static_cast<float>(c10.g), fx);
            const float b0 = lerp(static_cast<float>(c00.b), static_cast<float>(c10.b), fx);
            const float a0 = lerp(static_cast<float>(c00.a), static_cast<float>(c10.a), fx);
            const float r1 = lerp(static_cast<float>(c01.r), static_cast<float>(c11.r), fx);
            const float g1 = lerp(static_cast<float>(c01.g), static_cast<float>(c11.g), fx);
            const float b1 = lerp(static_cast<float>(c01.b), static_cast<float>(c11.b), fx);
            const float a1 = lerp(static_cast<float>(c01.a), static_cast<float>(c11.a), fx);

            output.set_pixel(
                x, y,
                renderer2d::Color{
                    static_cast<std::uint8_t>(std::clamp(std::lround(lerp(r0, r1, fy)), 0L, 255L)),
                    static_cast<std::uint8_t>(std::clamp(std::lround(lerp(g0, g1, fy)), 0L, 255L)),
                    static_cast<std::uint8_t>(std::clamp(std::lround(lerp(b0, b1, fy)), 0L, 255L)),
                    static_cast<std::uint8_t>(std::clamp(std::lround(lerp(a0, a1, fy)), 0L, 255L))
                });
        }
    }

    return output;
}

} // namespace

EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    const scene::EvaluationVariables vars{
        plan.variables.empty() ? nullptr : &plan.variables,
        plan.string_variables.empty() ? nullptr : &plan.string_variables
    };
    const auto evaluated = scene::evaluate_scene_composition_state(scene, plan.composition.id, task.frame_number, nullptr, vars);
    if (!evaluated.has_value()) {
        throw std::runtime_error("failed to evaluate composition state for frame");
    }

    EvaluatedFrameState state;
    state.task = task;
    state.composition_state = *evaluated;
    state.scene_hash = compiled_scene.scene_hash;
    state.composition_summary = build_summary(state.composition_state);
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    return renderer2d::DrawListBuilder::build(state.composition_state);
}

static void resolve_3d_layers_textures(
    std::vector<scene::EvaluatedLayerState>& layers,
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) { (void)scene; (void)plan; 
    (void)task;
    (void)cache;

    for (auto& layer : layers) {
        if (!layer.is_3d || !layer.visible) continue;

        if (layer.type == scene::LayerType::Image && layer.asset_path.has_value() && !layer.asset_path->empty()) {
            const auto* surface = context.media.get_image(std::filesystem::path(*layer.asset_path), media::AlphaMode::Straight);
            if (surface) {
                layer.texture_rgba = reinterpret_cast<const std::uint8_t*>(surface->pixels().data());
                layer.width  = static_cast<std::int64_t>(surface->width());
                layer.height = static_cast<std::int64_t>(surface->height());
            }
        }
    }
}

ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) { 
    
    const std::uint64_t scene_hash = compiled_scene.scene_hash;

    if (const CachedFrame* cached = cache.lookup(task.cache_key, scene_hash)) {
        ExecutedFrame frame;
        frame.frame_number = task.frame_number;
        frame.cache_key = task.cache_key;
        frame.cache_hit = true;
        frame.scene_hash = scene_hash;
        frame.draw_command_count = 0;
        frame.frame = cached->frame;
        return frame;
    }

    const EvaluatedFrameState state = evaluate_frame_state(scene, compiled_scene, plan, task);
    renderer2d::DrawList2D draw_list = build_draw_list(state);
    
    renderer2d::TextureResolver::resolve_textures(draw_list, scene, context.media);

    const float resolution_scale = std::clamp(context.policy.resolution_scale, 0.1f, 1.0f);
    const bool scaled_render = resolution_scale < 0.999f;
    const bool mb_enabled = plan.motion_blur_enabled;
    int mb_samples = mb_enabled ? static_cast<int>(plan.motion_blur_samples) : 1;
    mb_samples = std::clamp(mb_samples, 1, context.policy.motion_blur_sample_cap);
    
    const float shutter_angle = mb_enabled ? static_cast<float>(plan.motion_blur_shutter_angle) : 0.0f;
    const float shutter_duration = (shutter_angle / 360.0f) * (1.0f / static_cast<float>(plan.composition.frame_rate.value()));

    std::vector<float> accum_r(static_cast<std::size_t>(state.composition_state.width) * state.composition_state.height, 0.0f);
    std::vector<float> accum_g = accum_r;
    std::vector<float> accum_b = accum_r;
    std::vector<float> accum_a = accum_r;

    for (int s = 0; s < mb_samples; ++s) {
        float t_offset = (mb_samples > 1) ? (static_cast<float>(s) / (mb_samples - 1) - 0.5f) * shutter_duration : 0.0f;
        double subframe_time = state.composition_state.composition_time_seconds + t_offset;

        const scene::EvaluationVariables vars{
            plan.variables.empty() ? nullptr : &plan.variables,
            plan.string_variables.empty() ? nullptr : &plan.string_variables
        };
        const auto sub_evaluated = scene::evaluate_scene_composition_state(scene, plan.composition.id, subframe_time, nullptr, vars);
        if (!sub_evaluated.has_value()) continue;

        scene::EvaluatedCompositionState render_state = *sub_evaluated;
        resolve_3d_layers_textures(render_state.layers, scene, plan, task, cache, context);

        if (scaled_render) {
            render_state.width = std::max<std::int64_t>(static_cast<std::int64_t>(1), static_cast<std::int64_t>(std::lround(static_cast<double>(render_state.width) * resolution_scale)));
            render_state.height = std::max<std::int64_t>(static_cast<std::int64_t>(1), static_cast<std::int64_t>(std::lround(static_cast<double>(render_state.height) * resolution_scale)));
        }

        int original_spp = context.policy.ray_tracer_spp;
        if (mb_samples > 1) {
            context.renderer2d.policy.ray_tracer_spp = std::max(1, original_spp / mb_samples);
        }
        
        context.renderer2d.policy = context.policy;
        const RasterizedFrame2D rasterized = tachyon::render_evaluated_composition_2d(render_state, plan, task, context.renderer2d);
        
        if (rasterized.surface.has_value()) {
            const auto& surface = *rasterized.surface;
            for (std::uint32_t y = 0; y < surface.height(); ++y) {
                for (std::uint32_t x = 0; x < surface.width(); ++x) {
                    const auto px = surface.get_pixel(x, y);
                    const std::size_t idx = static_cast<std::size_t>(y) * surface.width() + x;
                    accum_r[idx] += static_cast<float>(px.r) / (255.0f * mb_samples);
                    accum_g[idx] += static_cast<float>(px.g) / (255.0f * mb_samples);
                    accum_b[idx] += static_cast<float>(px.b) / (255.0f * mb_samples);
                    accum_a[idx] += static_cast<float>(px.a) / (255.0f * mb_samples);
                }
            }
        }
    }

    renderer2d::Framebuffer frame(
        static_cast<std::uint32_t>(state.composition_state.width),
        static_cast<std::uint32_t>(state.composition_state.height));
    
    for (std::uint32_t y = 0; y < frame.height(); ++y) {
        for (std::uint32_t x = 0; x < frame.width(); ++x) {
            const std::size_t idx = static_cast<std::size_t>(y) * frame.width() + x;
            frame.set_pixel(x, y, renderer2d::Color{
                static_cast<std::uint8_t>(std::clamp(std::lround(accum_r[idx] * 255.0f), 0L, 255L)),
                static_cast<std::uint8_t>(std::clamp(std::lround(accum_g[idx] * 255.0f), 0L, 255L)),
                static_cast<std::uint8_t>(std::clamp(std::lround(accum_b[idx] * 255.0f), 0L, 255L)),
                static_cast<std::uint8_t>(std::clamp(std::lround(accum_a[idx] * 255.0f), 0L, 255L))
            });
        }
    }

    if (scaled_render) {
        frame = resize_frame_bilinear(
            frame,
            static_cast<std::uint32_t>(state.composition_state.width),
            static_cast<std::uint32_t>(state.composition_state.height));
    }

    if (task.cacheable) {
        cache.store(CachedFrame{
            FrameCacheEntry{task.cache_key, state.composition_summary},
            state.scene_hash,
            frame,
            task.invalidates_when_changed
        });
    }

    ExecutedFrame executed;
    executed.frame_number = task.frame_number;
    executed.cache_key = task.cache_key;
    executed.cache_hit = false;
    executed.scene_hash = state.scene_hash;
    executed.draw_command_count = draw_list.commands.size();
    executed.frame = std::move(frame);
    return executed;
}

} // namespace tachyon
