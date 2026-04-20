#include "tachyon/runtime/frame_executor.h"

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

void append_part(std::ostringstream& stream, const char* key, const std::string& value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, std::int64_t value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, double value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, bool value) {
    stream << key << '=' << (value ? 1 : 0) << ';';
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<std::string>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<double>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

std::string build_scene_signature(const SceneSpec& scene) {
    std::ostringstream stream;
    append_part(stream, "spec_version", scene.spec_version);
    append_part(stream, "project.id", scene.project.id);
    append_part(stream, "project.name", scene.project.name);
    append_part(stream, "asset_count", static_cast<std::int64_t>(scene.assets.size()));
    for (const auto& asset : scene.assets) {
        append_part(stream, "asset.id", asset.id);
        append_part(stream, "asset.type", asset.type);
        append_part(stream, "asset.source", asset.source);
    }

    append_part(stream, "composition_count", static_cast<std::int64_t>(scene.compositions.size()));
    for (const auto& composition : scene.compositions) {
        append_part(stream, "composition.id", composition.id);
        append_part(stream, "composition.name", composition.name);
        append_part(stream, "composition.width", composition.width);
        append_part(stream, "composition.height", composition.height);
        append_part(stream, "composition.duration", composition.duration);
        append_part(stream, "composition.frame_rate.numerator", composition.frame_rate.numerator);
        append_part(stream, "composition.frame_rate.denominator", composition.frame_rate.denominator);
        append_optional(stream, "composition.background", composition.background);
        append_part(stream, "composition.layer_count", static_cast<std::int64_t>(composition.layers.size()));

        for (const auto& layer : composition.layers) {
            append_part(stream, "layer.id", layer.id);
            append_part(stream, "layer.type", layer.type);
            append_part(stream, "layer.name", layer.name);
            append_part(stream, "layer.blend_mode", layer.blend_mode);
            append_part(stream, "layer.enabled", layer.enabled);
            append_part(stream, "layer.start_time", layer.start_time);
            append_part(stream, "layer.in_point", layer.in_point);
            append_part(stream, "layer.out_point", layer.out_point);
            append_part(stream, "layer.opacity", layer.opacity);
            append_optional(stream, "layer.parent", layer.parent);
            append_optional(stream, "layer.transform.position_x", layer.transform.position_x);
            append_optional(stream, "layer.transform.position_y", layer.transform.position_y);
            append_optional(stream, "layer.transform.rotation", layer.transform.rotation);
            append_optional(stream, "layer.transform.scale_x", layer.transform.scale_x);
            append_optional(stream, "layer.transform.scale_y", layer.transform.scale_y);

            append_part(stream, "layer.opacity_keyframes", static_cast<std::int64_t>(layer.opacity_property.keyframes.size()));
            append_optional(stream, "layer.opacity_value", layer.opacity_property.value);
            append_part(stream, "layer.time_remap_keyframes", static_cast<std::int64_t>(layer.time_remap_property.keyframes.size()));
            append_optional(stream, "layer.time_remap_value", layer.time_remap_property.value);
            append_part(stream, "layer.transform.position_keyframes", static_cast<std::int64_t>(layer.transform.position_property.keyframes.size()));
            if (layer.transform.position_property.value.has_value()) {
                append_part(stream, "layer.transform.position_value_x", layer.transform.position_property.value->x);
                append_part(stream, "layer.transform.position_value_y", layer.transform.position_property.value->y);
            }
            append_part(stream, "layer.transform.rotation_keyframes", static_cast<std::int64_t>(layer.transform.rotation_property.keyframes.size()));
            append_optional(stream, "layer.transform.rotation_value", layer.transform.rotation_property.value);
            append_part(stream, "layer.transform.scale_keyframes", static_cast<std::int64_t>(layer.transform.scale_property.keyframes.size()));
            if (layer.transform.scale_property.value.has_value()) {
                append_part(stream, "layer.transform.scale_value_x", layer.transform.scale_property.value->x);
                append_part(stream, "layer.transform.scale_value_y", layer.transform.scale_property.value->y);
            }
        }
    }

    return stream.str();
}

std::string build_fingerprint(const scene::EvaluatedCompositionState& state) {
    std::ostringstream stream;
    stream << state.composition_id << ';' << state.frame_number << ';' << state.width << 'x' << state.height << ';';
    for (const auto& layer : state.layers) {
        stream << layer.id << ':' << layer.active << ':' << layer.opacity << ':'
               << layer.local_transform.position.x << ':' << layer.local_transform.position.y << ':'
               << layer.local_transform.scale.x << ':' << layer.local_transform.scale.y << ';';
    }
    return stream.str();
}

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

EvaluatedFrameState evaluate_frame_state(const SceneSpec& scene, const RenderPlan& plan, const FrameRenderTask& task) {
    return evaluate_frame_state(scene, plan, task, build_scene_signature(scene));
}

EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const std::string& scene_signature) {

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
    state.scene_signature = scene_signature;
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

        // 1. Resolve Image Asset textures
        if (layer.type == scene::LayerType::Image && layer.asset_path.has_value() && !layer.asset_path->empty()) {
            const auto* surface = context.media.get_image(std::filesystem::path(*layer.asset_path), media::AlphaMode::Straight);
            if (surface) {
                layer.texture_rgba = reinterpret_cast<const std::uint8_t*>(surface->pixels().data());
                layer.width  = static_cast<std::int64_t>(surface->width());
                layer.height = static_cast<std::int64_t>(surface->height());
            }
        }
        
        // 2. Resolve Precomp textures by rendering them
        else if (layer.type == scene::LayerType::Precomp && layer.precomp_id.has_value() && !layer.precomp_id->empty()) {
            // We need to render the nested comp into a buffer
            // For now, let's look for it in the cache or render it on the fly
            // This is a simplified version of Ae's behavior
            // In a full implementation, we'd have a recursive render call here
            
            // TODO: Full recursive render for 3D precomp textures
            // For now, we only support Images as 3D textures in this hardening phase
        }
    }
}

ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) { (void)scene; (void)plan; 
    (void)context;
    const std::string scene_signature = build_scene_signature(scene);

    if (const CachedFrame* cached = cache.lookup(task.cache_key, scene_signature)) {
        ExecutedFrame frame;
        frame.frame_number = task.frame_number;
        frame.cache_key = task.cache_key;
        frame.cache_hit = true;
        frame.scene_signature = scene_signature;
        frame.draw_command_count = 0;
        frame.frame = cached->frame;
        return frame;
    }

    const EvaluatedFrameState state = evaluate_frame_state(scene, plan, task, scene_signature);
    renderer2d::DrawList2D draw_list = build_draw_list(state);
    
    // Resolve textures using MediaManager
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

        // Re-evaluate state for this subframe
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

        // Adjust SPP if multiple subframes
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
            state.scene_signature,
            frame,
            task.invalidates_when_changed
        });
    }

    ExecutedFrame executed;
    executed.frame_number = task.frame_number;
    executed.cache_key = task.cache_key;
    executed.cache_hit = false;
    executed.scene_signature = state.scene_signature;
    executed.draw_command_count = draw_list.commands.size();
    executed.frame = std::move(frame);
    return executed;
}

} // namespace tachyon
