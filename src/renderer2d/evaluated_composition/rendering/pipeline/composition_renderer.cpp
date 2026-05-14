#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/runtime/execution/session/render_internal.h"

#ifdef _MSC_VER
#pragma warning(disable:4456)
#endif
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/evaluated_composition/renderer2d_matte_resolver.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"

#include "tachyon/core/render/dof_settings.h"
#include "tachyon/core/render/motion_blur_settings.h"
#include "tachyon/core/render/visibility.h"
#include "tachyon/renderer2d/raster/tile_grid.h"
#include "tachyon/output/frame_aov.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/transition/transition_resolver.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_apply.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/core/transition/transition_fast_path_registry.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <array>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <atomic>
#include <mutex>

#include "tachyon/core/profiling.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon {

namespace {

template <typename ClockPoint>
void record_timing(
    FrameDiagnostics* diagnostics,
    const char* category,
    const std::string& label,
    const ClockPoint& start) {
    if (!diagnostics) {
        return;
    }
    const auto end = std::chrono::high_resolution_clock::now();
    static std::mutex s_timing_mutex;
    std::lock_guard<std::mutex> lock(s_timing_mutex);
    diagnostics->timings.push_back(TimingSample{
        category,
        label,
        std::chrono::duration<double, std::milli>(end - start).count()
    });
}

    ResolvedTransition resolve_layer_transition(const LayerTransitionSpec& transition, const TransitionRegistry& registry) {
        return resolve_transition_spec(transition, registry);
    }

std::optional<double> compute_transition_progress(double elapsed_seconds, double duration_seconds) {
    if (duration_seconds <= 0.0 || elapsed_seconds < 0.0 || elapsed_seconds >= duration_seconds) {
        return std::nullopt;
    }
    return std::clamp(elapsed_seconds / duration_seconds, 0.0, 1.0);
}

static TransitionRegistry s_fallback_registry;

} // namespace
using namespace renderer2d;

std::optional<std::filesystem::path> resolve_media_source(
    const scene::EvaluatedLayerState& layer,
    const RenderContext& context) {

    if (!context.media) {
        return std::nullopt;
    }

    const std::array<std::string, 3> references = {
        layer.asset_path.value_or(""),
        layer.id,
        layer.name
    };

    for (const auto& reference : references) {
        if (reference.empty()) continue;

        auto path = context.media->resolve_media_path(reference);
        if (!path.empty()) {
            return path;
        }

        // Fallback: check if the reference is already a valid absolute path
        std::filesystem::path candidate(reference);
        if (candidate.is_absolute() && std::filesystem::exists(candidate)) {
             return candidate;
        }
        
        // Fallback: check if it's a relative path that exists
        if (std::filesystem::exists(candidate)) {
             return std::filesystem::absolute(candidate);
        }

    }

    return std::nullopt;
}

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    [[maybe_unused]] const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    [[maybe_unused]] const renderer2d::EffectRegistry& effect_registry,
    std::shared_ptr<renderer2d::SurfaceRGBA> output_surface) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = state.width;
    frame.height = state.height;
    frame.layer_count = state.layers.size();
    frame.backend_name = "cpu-evaluated-composition";
    frame.cache_key = task.cache_key.value;
    frame.note = "Frame rasterized from evaluated composition";
    
    const float res_scale = plan.quality_policy.resolution_scale;
    const std::uint32_t working_width = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<float>(state.width) * res_scale))));
    const std::uint32_t working_height = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<float>(state.height) * res_scale))));

    // Use the pre-allocated output surface when the caller provides one at the right size.
    // This eliminates the full-frame blit in RasterizationStep.
    if (output_surface
        && output_surface->width() == working_width
        && output_surface->height() == working_height) {
        frame.surface = std::move(output_surface);
    } else {
        frame.surface = context.surface_pool
            ? context.surface_pool->acquire(working_width, working_height)
            : std::make_shared<SurfaceRGBA>(working_width, working_height);
    }
    if (!frame.surface) {
        frame.note += " [ERROR: failed to allocate surface of size " +
                     std::to_string(working_width) + "x" + std::to_string(working_height) + "]";
        return frame;
    }

    frame.surface->clear(Color::transparent());

    auto& dst = *frame.surface;
    dst.set_profile(context.cms.working_profile);
    dst.clear_depth(0.0f);

    FrameDiagnostics* diagnostics = context.diagnostics;

    // First pass: collect rendered surfaces for matte resolution and cross-layer effect references.
    // Second pass: composite with resolved mattes, reusing first-pass surfaces where possible.
    std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> rendered_surfaces;
    std::vector<std::shared_ptr<SurfaceRGBA>> layer_surface_cache(state.layers.size());
    std::vector<MatteDependency> matte_dependencies;

    // Build matte dependency list from layers
    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (layer.track_matte_layer_index.has_value() && layer.track_matte_type != TrackMatteType::None) {
            const auto matte_idx = *layer.track_matte_layer_index;
            if (matte_idx < state.layers.size() && !state.layers[matte_idx].id.empty() && !layer.id.empty()) {
                MatteDependency dep;
                dep.source_layer_id = state.layers[matte_idx].id;
                dep.target_layer_id = layer.id;
                // Map TrackMatteType to MatteMode
                switch (layer.track_matte_type) {
                    case TrackMatteType::Alpha: dep.mode = MatteMode::Alpha; break;
                    case TrackMatteType::AlphaInverted: dep.mode = MatteMode::AlphaInverted; break;
                    case TrackMatteType::Luma: dep.mode = MatteMode::Luma; break;
                    case TrackMatteType::LumaInverted: dep.mode = MatteMode::LumaInverted; break;
                    default: dep.mode = MatteMode::Alpha; break;
                }
                matte_dependencies.push_back(dep);
            }
        }
    }

    // The first pass is only needed when matte dependencies exist or when effects / adjustment
    // layers require access to other layers' rendered output via the rendered_surfaces map.
    const bool needs_first_pass = !matte_dependencies.empty() || std::any_of(
        state.layers.begin(), state.layers.end(),
        [](const auto& l) {
            return l.enabled && l.active
                && (!l.effects.empty() || l.is_adjustment_layer);
        });

    if (needs_first_pass) {
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            if (context.cancel_flag && context.cancel_flag->load()) break;
            const auto& layer = state.layers[i];
            if (!layer.enabled || !layer.active || layer.id.empty()) {
                continue;
            }

            const auto layer_surface_start = std::chrono::high_resolution_clock::now();
            auto layer_surface = render_layer_surface(layer, state, intent, plan, task, context, std::nullopt);
            record_timing(diagnostics, "layer_surface", layer.id.empty() ? std::to_string(i) : layer.id, layer_surface_start);

            if (layer_surface) {
                rendered_surfaces[layer.id] = layer_surface;
                layer_surface_cache[i] = layer_surface;
            }
        }
    }

    // Resolve matte dependencies using MatteResolver
    Renderer2DMatteResolver resolver;
    std::string validation_error;
    if (!matte_dependencies.empty() && !resolver.validate(state.layers, matte_dependencies, validation_error)) {
        frame.note += " [matte validation warning: " + validation_error + "]";
        if (context.diagnostics) {
            context.diagnostics->diagnostics.add_warning("matte_validation", validation_error);
        }
    }

    std::vector<std::vector<float>> matte_buffers;
    if (!matte_dependencies.empty()) {
        resolver.resolve(state.layers, matte_dependencies, rendered_surfaces, matte_buffers, state.width, state.height);
    }

    auto render_pass = [&](SurfaceRGBA& target_surface, RenderContext& render_context, const std::optional<RectI>& tile_rect = std::nullopt) {
        if (!render_context.effects) {
            render_context.effects = renderer2d::create_effect_host(effect_registry);
        }
        EffectHost& host = effect_host_for(render_context);
        
        // Render layers in stack order
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            if (render_context.cancel_flag && render_context.cancel_flag->load()) break;
            const auto& layer = state.layers[i];
            
            if (!layer.enabled || !layer.active || layer.id.empty()) {
                continue;
            }

            // Adjustment layers operate on target_surface directly — no per-layer surface needed.
            // For all other layers: reuse the first-pass surface when available (non-tiled path only).
            // Surfaces that will be mutated in-place are shallow-copied to preserve the cache.
            std::shared_ptr<SurfaceRGBA> layer_surface;
            if (!layer.is_adjustment_layer) {
                if (!tile_rect.has_value() && i < layer_surface_cache.size() && layer_surface_cache[i]) {
                    const bool has_transition_spec =
                        (!layer.transition_in.transition_id.empty()
                         || (!layer.transition_in.type.empty() && layer.transition_in.type != "none"))
                        || (!layer.transition_out.transition_id.empty()
                         || (!layer.transition_out.type.empty() && layer.transition_out.type != "none"));
                    const bool will_mutate = !layer.effects.empty()
                        || has_transition_spec
                        || layer.opacity < 0.9999f
                        || (i < matte_buffers.size() && !matte_buffers[i].empty());
                    layer_surface = will_mutate
                        ? std::make_shared<SurfaceRGBA>(*layer_surface_cache[i])
                        : layer_surface_cache[i];
                } else {
                    layer_surface = render_layer_surface(layer, state, intent, plan, task, render_context, tile_rect);
                }
                if (!layer_surface) {
                    continue;
                }
            }

            if (layer.is_adjustment_layer) {
                if (render_context.policy.effects_enabled) {
                    const auto adjustment_start = std::chrono::high_resolution_clock::now();
                    auto res = apply_effect_pipeline(
                        target_surface,
                        layer.effects,
                        host,
                        render_context.working_color_space.profile,
                        rendered_surfaces,
                        layer.id,
                        render_context.diagnostics);
                    record_timing(render_context.diagnostics, "adjustment", layer.id, adjustment_start);
                    
                    if (res.ok()) {
                        auto& adjusted = *res.value;
                        multiply_surface_alpha(adjusted, static_cast<float>(layer.opacity));
                        composite_surface(target_surface, adjusted, 0, 0, BlendMode::Normal);
                    }
                }
                continue;
            }

            // Effects
            if (render_context.policy.effects_enabled && !layer.effects.empty()) {
                const auto effects_start = std::chrono::high_resolution_clock::now();
                auto res = apply_effect_pipeline(
                    *layer_surface,
                    layer.effects,
                    host,
                    render_context.working_color_space.profile,
                    rendered_surfaces,
                    layer.id,
                    render_context.diagnostics);
                record_timing(render_context.diagnostics, "effect_pipeline", layer.id, effects_start);
                
                if (res.ok()) {
                    *layer_surface = std::move(*res.value);
                } else {
                    layer_surface->clear(Color::transparent());
                }
            }

            // Layer transitions are semantic, not optional post effects.
            // Keep them active even when the quality policy disables heavier effect passes.
            const double layer_time = task.time_seconds;
            bool in_transition = false;
            bool out_transition = false;
            double transition_t = 0.0;
            ResolvedTransition resolution;

            // Check transition_in
            if (!layer.transition_in.transition_id.empty() || layer.transition_in.type != "none") {
                const double relative_time = layer_time - layer.in_time;
                const double transition_duration = layer.transition_in.duration;
                const double start_time = layer.transition_in.delay;
                const auto progress = compute_transition_progress(relative_time - start_time, transition_duration);
                if (progress.has_value()) {
                    in_transition = true;
                    transition_t = animation::apply_easing(*progress, layer.transition_in.easing, {});
                    const TransitionRegistry& registry = render_context.transition_registry ? *render_context.transition_registry : s_fallback_registry;
                    resolution = resolve_layer_transition(layer.transition_in, registry);
                }
            }

            // Check transition_out
            if (!in_transition && (!layer.transition_out.transition_id.empty() || layer.transition_out.type != "none")) {
                const double time_until_end = layer.out_time - layer_time;
                const double transition_duration = layer.transition_out.duration;
                const auto progress = compute_transition_progress(time_until_end, transition_duration);
                if (progress.has_value()) {
                    out_transition = true;
                    transition_t = animation::apply_easing(*progress, layer.transition_out.easing, {});
                    const TransitionRegistry& registry = render_context.transition_registry ? *render_context.transition_registry : s_fallback_registry;
                    resolution = resolve_layer_transition(layer.transition_out, registry);
                }
            }

            if ((in_transition || out_transition) && transition_t > 0.0) {
                if (resolution.valid) {
                    TransitionApplyRequest apply_request;
                    apply_request.transition_id = resolution.descriptor->id;
                    
                    if (in_transition) {
                        apply_request.from = nullptr; // apply_transition treats null from as error, wait
                        // Actually, for IN transition, we are transitioning FROM nothing TO the layer.
                        // So 'from' should be transparent, 'to' should be the layer.
                        apply_request.from = nullptr; // I need to fix apply_transition to handle null from too
                        apply_request.to = layer_surface.get();
                    } else {
                        apply_request.from = layer_surface.get();
                        apply_request.to = nullptr; // Out transition: layer to nothing
                    }

                    apply_request.progress = static_cast<float>(transition_t);
                    apply_request.thread_count = 1;
#ifdef _OPENMP
                    apply_request.thread_count = omp_get_max_threads();
#endif

                    const TransitionRegistry& registry = render_context.transition_registry ? *render_context.transition_registry : s_fallback_registry;
                    auto apply_res = apply_transition(apply_request, registry);
                    
                    if (apply_res.ok) {
                        auto transition_result = render_context.surface_pool 
                            ? render_context.surface_pool->acquire(layer_surface->width(), layer_surface->height())
                            : std::make_shared<SurfaceRGBA>(layer_surface->width(), layer_surface->height());
                        *transition_result = std::move(apply_res.output);
                        layer_surface = transition_result;
                    }
                }
            }

            // Opacity
            multiply_surface_alpha(*layer_surface, static_cast<float>(layer.opacity));

            // Apply resolved matte if available
            if (i < matte_buffers.size() && !matte_buffers[i].empty()) {
                ::tachyon::renderer2d::apply_matte_buffer(*layer_surface, matte_buffers[i], state.width, state.height);
            }

            // Final Composite
            composite_surface(target_surface, *layer_surface, 0, 0, parse_blend_mode(layer.blend_mode), -1.0f);
        }
    };

    const bool has_effectful_layers = std::any_of(
        state.layers.begin(),
        state.layers.end(),
        [](const auto& layer) {
            bool has_transition = (!layer.transition_in.transition_id.empty() && layer.transition_in.type != "none") ||
                                  (!layer.transition_out.transition_id.empty() && layer.transition_out.type != "none");
            return layer.enabled
                && layer.active
                && (!layer.effects.empty() || has_transition);
        });

    const bool should_tile = context.policy.tile_size > 0 && !has_effectful_layers;

    if (should_tile) {
        TileGrid grid = build_tile_grid({0, 0, static_cast<int>(working_width), static_cast<int>(working_height)}, working_width, working_height, context.policy.tile_size);

        auto render_tile = [&](std::size_t i) {
            TACHYON_ZONE("RenderTile");
            const auto& tile = grid.tiles[i];
            auto tile_surface_ptr = context.surface_pool
                ? context.surface_pool->acquire(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height))
                : std::make_shared<SurfaceRGBA>(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
            tile_surface_ptr->clear(Color::transparent());
            RenderContext thread_context = context;
            render_pass(*tile_surface_ptr, thread_context, tile);
            composite_surface(dst, *tile_surface_ptr, tile.x, tile.y, BlendMode::Normal);
        };

        if (context.tile_executor) {
            context.tile_executor(grid.tiles.size(), render_tile);
        } else {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
            for (int i = 0; i < static_cast<int>(grid.tiles.size()); ++i) {
                render_tile(static_cast<std::size_t>(i));
            }
        }
    } else {
        render_pass(dst, context);
    }

    return frame;
}

} // namespace tachyon
