#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"

#ifdef _MSC_VER
#pragma warning(disable:4456)
#endif
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/evaluated_composition/renderer2d_matte_resolver.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.h"
#include "tachyon/renderer2d/raster/mesh_rasterizer.h"
#include "tachyon/renderer2d/raster/perspective_rasterizer.h"
#include "tachyon/renderer2d/raster/mesh_deform_apply.h"
#include "tachyon/core/render/iray_tracer.h"
#include "tachyon/core/render/aov_buffer.h"
#include "tachyon/core/render/dof_settings.h"
#include "tachyon/core/render/motion_blur_settings.h"
#include "tachyon/core/render/visibility.h"
#include "tachyon/renderer2d/raster/tile_grid.h"
#include "tachyon/output/frame_aov.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/transition/transition_resolver.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/deform/mesh_deform.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <array>
#include <chrono>
#include <filesystem>
#include <unordered_map>

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
    diagnostics->timings.push_back(TimingSample{
        category,
        label,
        std::chrono::duration<double, std::milli>(end - start).count()
    });
}

ResolvedTransition resolve_layer_transition(const LayerTransitionSpec& transition, const TransitionRegistry* registry) {
    if (!registry) return {};
    return resolve_transition_spec(transition, *registry);
}

std::optional<double> compute_transition_progress(double elapsed_seconds, double duration_seconds) {
    if (duration_seconds <= 0.0 || elapsed_seconds < 0.0 || elapsed_seconds >= duration_seconds) {
        return std::nullopt;
    }
    return std::clamp(elapsed_seconds / duration_seconds, 0.0, 1.0);
}

} // namespace
using namespace renderer2d;

std::optional<std::filesystem::path> resolve_media_source(
    const scene::EvaluatedLayerState& layer,
    const RenderContext2D& context) {

    if (!context.media_manager) {
        return std::nullopt;
    }

    const std::array<std::string, 3> references = {
        layer.asset_path.value_or(""),
        layer.id,
        layer.name
    };

    int ref_idx = 0;
    for (const auto& reference : references) {
        if (reference.empty()) {
            ref_idx++;
            continue;
        }

        std::filesystem::path resolved = context.media_manager->get_asset_path(reference);
        if (!resolved.empty()) {
            if (ref_idx > 0 && context.diagnostics) {
                context.diagnostics->add_warning("asset_fallback", 
                    "Media source resolved via fallback reference '" + reference + "' (original asset_path was missing or invalid).", 
                    layer.id);
            }
            return resolved;
        }

        std::filesystem::path candidate(reference);
        if (candidate.has_extension()) {
            if (ref_idx > 0 && context.diagnostics) {
                context.diagnostics->add_warning("asset_fallback", 
                    "Media source resolved via direct path fallback '" + reference + "'.", 
                    layer.id);
            }
            return candidate;
        }
        ref_idx++;
    }

    if (context.diagnostics) {
        context.diagnostics->add_error("asset_missing", 
            "Failed to resolve media source for layer '" + layer.name + "' using asset_path, ID, or name.", 
            layer.id);
    }
    return std::nullopt;
}

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context,
    const EffectRegistry& effect_registry) {
    (void)intent;

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

    frame.surface = std::make_shared<SurfaceRGBA>(working_width, working_height);
    if (!frame.surface) {
        frame.note += " [ERROR: failed to allocate surface of size " + 
                     std::to_string(working_width) + "x" + std::to_string(working_height) + "]";
        return frame;
    }

    // Clear background to transparent.
    // Convergence: Background is now a standard Layer at index 0, 
    // responsible for filling the surface if present.
    frame.surface->clear(Color::transparent());
    
    auto& dst = *frame.surface;
    dst.set_profile(context.cms.working_profile);
    dst.clear_depth(0.0f); // Initialize depth buffer for hybrid compositing

    FrameDiagnostics* diagnostics = context.diagnostics;

#ifdef TACHYON_ENABLE_3D
    bool has_any_3d = std::any_of(state.layers.begin(), state.layers.end(), [](const auto& l) { return l.is_3d && l.visible; });
#endif

    std::vector<bool> visible_3d_layers(state.layers.size(), true);
#ifdef TACHYON_ENABLE_3D
    if (has_any_3d && state.camera.available) {
        renderer3d::SceneCulling culling;
        culling.set_camera(state.camera.camera, state.camera.position);
        const auto culling_result = culling.cull_layers(state, &intent);
        visible_3d_layers.assign(state.layers.size(), false);
        if (culling_result.visibleIndices.empty()) {
            for (std::size_t i = 0; i < state.layers.size(); ++i) {
                if (state.layers[i].is_3d && state.layers[i].visible) {
                    visible_3d_layers[i] = true;
                }
            }
        } else {
            for (const auto index : culling_result.visibleIndices) {
                if (index < visible_3d_layers.size()) {
                    visible_3d_layers[index] = true;
                }
            }
        }
    }
#endif

    // First pass: collect rendered surfaces for matte resolution
    // Second pass: composite with resolved mattes
    std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> rendered_surfaces;
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

    // Render all layers and collect surfaces
    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        if (context.cancel_flag && context.cancel_flag->load()) break;
        const auto& layer = state.layers[i];
        if (!layer.enabled || !layer.active || layer.id.empty()) {
            continue;
        }

        // Skip 3D layers for now (they're handled separately)
        if (layer.is_3d && layer.visible) {
            continue;
        }

        const auto layer_surface_start = std::chrono::high_resolution_clock::now();
        auto layer_surface = render_layer_surface(layer, state, intent, plan, task, context, std::nullopt);
        record_timing(diagnostics, "layer_surface", layer.id.empty() ? std::to_string(i) : layer.id, layer_surface_start);
        
        // Apply mesh deformation for matte resolution
        if (layer.mesh_deform_id.has_value() && layer_surface) {
            auto it = intent.layer_resources.find(layer.id);
            if (it != intent.layer_resources.end() && it->second.mesh_deform) {
                if (auto* mesh_ptr = dynamic_cast<const renderer2d::DeformMesh*>(it->second.mesh_deform.get())) {
                    layer_surface = apply_mesh_deform(
                        layer_surface,
                        *mesh_ptr,
                        layer.width,
                        layer.height);
                }
            }
        }
        
        if (layer_surface) {
            rendered_surfaces[layer.id] = layer_surface;
        }
    }

    // Resolve matte dependencies using MatteResolver
    Renderer2DMatteResolver resolver;
    std::string validation_error;
    if (!matte_dependencies.empty() && !resolver.validate(state.layers, matte_dependencies, validation_error)) {
        frame.note += " [matte validation warning: " + validation_error + "]";
        if (context.diagnostics) {
            context.diagnostics->add_warning("matte_validation", validation_error);
        }
    }

    std::vector<std::vector<float>> matte_buffers;
    if (!matte_dependencies.empty()) {
        resolver.resolve(state.layers, matte_dependencies, rendered_surfaces, matte_buffers, state.width, state.height);
    }

    auto render_pass = [&](SurfaceRGBA& target_surface, RenderContext2D& render_context, const std::optional<RectI>& tile_rect = std::nullopt) {
        target_surface.clear(Color::transparent());
        EffectHost& host = effect_host_for(render_context);
        // Render layers in stack order so higher layers can affect the composite below them.
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            if (render_context.cancel_flag && render_context.cancel_flag->load()) break;
            const auto& layer = state.layers[i];
            
            if (!layer.enabled || !layer.active || layer.id.empty()) {
                continue;
            }

            if (layer.is_3d && layer.visible) {
#ifdef TACHYON_ENABLE_3D
                // Found a 3D block. Collect contiguous 3D layers in stack order.
                std::vector<std::size_t> block_indices;
                block_indices.push_back(i);
                std::size_t last_block_idx = i;
                for (std::size_t j = i + 1; j < state.layers.size(); ++j) {
                    const auto& scan_layer = state.layers[j];
                    if (scan_layer.is_3d && scan_layer.visible && scan_layer.enabled && scan_layer.active) {
                        block_indices.push_back(j);
                        last_block_idx = j;
                    } else if (scan_layer.enabled && scan_layer.active) {
                        break;
                    }
                }

                // Construct EvaluatedScene3D bridge from the composition subset
                Scene3DBridgeInput bridge_input;
                bridge_input.state = &state;
                bridge_input.intent = &intent;
                bridge_input.plan = &plan;
                bridge_input.task = &task;
                bridge_input.context = &render_context;
                bridge_input.block_indices = &block_indices;
                bridge_input.visible_3d_layers = &visible_3d_layers;

                const auto bridge_output = build_evaluated_scene_3d(bridge_input);
                if (!bridge_output.has_instances) {
                    i = last_block_idx;
                    continue;
                }

                const std::uint32_t w = static_cast<std::uint32_t>(state.width);
                const std::uint32_t h = static_cast<std::uint32_t>(state.height);
                auto world_3d = std::make_shared<SurfaceRGBA>(w, h);
                world_3d->set_profile(render_context.cms.working_profile);
                world_3d->clear(renderer2d::Color::transparent());
                world_3d->clear_depth(0.0f);

                bool world_has_visible_pixels = false;

                // Convergence: Prefer high-quality ray tracing if IRayTracer is provided.
                if (render_context.ray_tracer) {
                    const auto ray_start = std::chrono::high_resolution_clock::now();
                    
                    AOVBuffer aovs;
                    aovs.resize(w, h);
                    
                    render_context.ray_tracer->build_scene(bridge_output.scene3d);
                    
                    double dt = 1.0 / 60.0;
                    if (plan.quality_policy.motion_blur_samples > 0) {
                        // FPS is used for temporal sample spacing
                    }

                    render_context.ray_tracer->render(bridge_output.scene3d, aovs, task.time_seconds, dt);
                    
                    // Transfer AOVs to SurfaceRGBA
                    #pragma omp parallel for schedule(static)
                    for (int y = 0; y < static_cast<int>(h); ++y) {
                        for (std::uint32_t x = 0; x < w; ++x) {
                            const std::size_t idx = (static_cast<std::size_t>(y) * w + x) * 4;
                            const std::size_t z_idx = static_cast<std::size_t>(y) * w + x;
                            
                            Color c;
                            c.r = aovs.beauty_rgba[idx];
                            c.g = aovs.beauty_rgba[idx+1];
                            c.b = aovs.beauty_rgba[idx+2];
                            c.a = aovs.beauty_rgba[idx+3];
                            
                            if (c.a > 0.0f) {
                                world_3d->set_pixel(x, static_cast<std::uint32_t>(y), c);
                                if (aovs.depth_z[z_idx] > 0.0f) {
                                    world_3d->test_and_write_depth(x, static_cast<std::uint32_t>(y), 1.0f / aovs.depth_z[z_idx]);
                                }
                                world_has_visible_pixels = true;
                            }
                        }
                    }
                    record_timing(diagnostics, "ray_tracing_block", std::to_string(i), ray_start);
                }

                if (!world_has_visible_pixels) {
                    const auto view_matrix = state.camera.camera.get_view_matrix();
                for (std::size_t block_idx : block_indices) {
                    if (block_idx >= state.layers.size()) {
                        continue;
                    }
                    const auto& block_layer = state.layers[block_idx];
                    if (!block_layer.visible || !block_layer.enabled || !block_layer.active || block_layer.width <= 0 || block_layer.height <= 0) {
                        continue;
                    }

                    std::shared_ptr<SurfaceRGBA> block_surface;
                    auto rendered_surface_it = rendered_surfaces.find(block_layer.id);
                    if (rendered_surface_it != rendered_surfaces.end()) {
                        block_surface = rendered_surface_it->second;
                    }
                    if (!block_surface) {
                        const auto block_surface_start = std::chrono::high_resolution_clock::now();
                        block_surface = render_layer_surface(block_layer, state, intent, plan, task, render_context, tile_rect);
                        record_timing(diagnostics, "layer_surface", block_layer.id.empty() ? std::to_string(block_idx) : block_layer.id, block_surface_start);
                    }
                    if (!block_surface) {
                        continue;
                    }

                    const float half_w = static_cast<float>(block_layer.width) * 0.5f;
                    const float half_h = static_cast<float>(block_layer.height) * 0.5f;
                    const math::Vector3 local_p0{-half_w, -half_h, 0.0f};
                    const math::Vector3 local_p1{ half_w, -half_h, 0.0f};
                    const math::Vector3 local_p2{ half_w,  half_h, 0.0f};
                    const math::Vector3 local_p3{-half_w,  half_h, 0.0f};

                    const math::Vector3 world_p0 = block_layer.world_matrix.transform_point(local_p0);
                    const math::Vector3 world_p1 = block_layer.world_matrix.transform_point(local_p1);
                    const math::Vector3 world_p2 = block_layer.world_matrix.transform_point(local_p2);
                    const math::Vector3 world_p3 = block_layer.world_matrix.transform_point(local_p3);

                    const math::Vector3 cam_p0 = view_matrix.transform_point(world_p0);
                    const math::Vector3 cam_p1 = view_matrix.transform_point(world_p1);
                    const math::Vector3 cam_p2 = view_matrix.transform_point(world_p2);
                    const math::Vector3 cam_p3 = view_matrix.transform_point(world_p3);

                    if (cam_p0.z > -state.camera.camera.near_z &&
                        cam_p1.z > -state.camera.camera.near_z &&
                        cam_p2.z > -state.camera.camera.near_z &&
                        cam_p3.z > -state.camera.camera.near_z) {
                        continue;
                    }

                    const math::Vector2 screen_p0 = state.camera.camera.project_point(world_p0, static_cast<float>(w), static_cast<float>(h));
                    const math::Vector2 screen_p1 = state.camera.camera.project_point(world_p1, static_cast<float>(w), static_cast<float>(h));
                    const math::Vector2 screen_p2 = state.camera.camera.project_point(world_p2, static_cast<float>(w), static_cast<float>(h));
                    const math::Vector2 screen_p3 = state.camera.camera.project_point(world_p3, static_cast<float>(w), static_cast<float>(h));

                    if ((screen_p0.x < 0.0f && screen_p0.y < 0.0f) &&
                        (screen_p1.x < 0.0f && screen_p1.y < 0.0f) &&
                        (screen_p2.x < 0.0f && screen_p2.y < 0.0f) &&
                        (screen_p3.x < 0.0f && screen_p3.y < 0.0f)) {
                        continue;
                    }

                    auto make_vertex = [&](const math::Vector3& cam_p, const math::Vector2& screen_p) {
                        renderer2d::raster::Vertex3D v;
                        v.position = {screen_p.x, screen_p.y, 0.0f};
                        v.uv = {0.0f, 0.0f};
                        v.one_over_w = cam_p.z != 0.0f ? 1.0f / std::max(0.001f, -cam_p.z) : 1.0f;
                        return v;
                    };

                    renderer2d::raster::PerspectiveWarpQuad quad;
                    quad.v0 = make_vertex(cam_p0, screen_p0);
                    quad.v1 = make_vertex(cam_p1, screen_p1);
                    quad.v2 = make_vertex(cam_p2, screen_p2);
                    quad.v3 = make_vertex(cam_p3, screen_p3);
                    quad.v0.uv = {0.0f, 1.0f};
                    quad.v1.uv = {1.0f, 1.0f};
                    quad.v2.uv = {1.0f, 0.0f};
                    quad.v3.uv = {0.0f, 0.0f};
                    quad.texture = block_surface.get();
                    quad.opacity = static_cast<float>(block_layer.opacity);
                    renderer2d::raster::PerspectiveRasterizer::draw_quad(*world_3d, quad);
                    world_has_visible_pixels = true;
                }
                } // end if (!world_has_visible_pixels)

                const auto composite_layer_it = std::find_if(
                    block_indices.begin(),
                    block_indices.end(),
                    [&](std::size_t block_idx) {
                        const auto& candidate = state.layers[block_idx];
                        return candidate.visible
                            && candidate.enabled
                            && candidate.active
                            && candidate.type != scene::LayerType::Camera
                            && candidate.type != scene::LayerType::Light
                            && candidate.type != scene::LayerType::NullLayer;
                    });
                if (composite_layer_it != block_indices.end() && world_has_visible_pixels) {
                    composite_surface(target_surface, *world_3d, 0, 0, BlendMode::Normal);
                } else if (composite_layer_it != block_indices.end()) {
                    frame.note += " [3D ray pass empty; using fallback 2D card preview]";
                    const auto& fallback_layer = state.layers[*composite_layer_it];
                    auto fallback_surface = std::make_shared<SurfaceRGBA>(state.width, state.height);
                    fallback_surface->set_profile(render_context.cms.working_profile);
                    fallback_surface->clear(renderer2d::Color{0.04f, 0.05f, 0.08f, 1.0f});

                    const int card_w = std::max(1, std::min(static_cast<int>(fallback_layer.width), static_cast<int>(state.width) - 160));
                    const int card_h = std::max(1, std::min(static_cast<int>(fallback_layer.height), static_cast<int>(state.height) - 160));
                    const int card_x = std::max(0, (static_cast<int>(state.width) - card_w) / 2);
                    const int card_y = std::max(0, (static_cast<int>(state.height) - card_h) / 2);
                    const auto fill = from_color_spec(fallback_layer.fill_color, render_context.cms.working_profile);
                    const renderer2d::Color border{0.92f, 0.95f, 1.0f, 1.0f};
                    const renderer2d::Color shadow{0.0f, 0.0f, 0.0f, 0.28f};

                    for (int y = card_y - 18; y < card_y + card_h + 18; ++y) {
                        if (y < 0 || y >= static_cast<int>(state.height)) continue;
                        for (int x = card_x - 18; x < card_x + card_w + 18; ++x) {
                            if (x < 0 || x >= static_cast<int>(state.width)) continue;
                            const bool in_shadow = x >= card_x - 8 && x < card_x + card_w + 8 && y >= card_y - 8 && y < card_y + card_h + 8;
                            if (in_shadow) {
                                auto px = fallback_surface->get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
                                fallback_surface->set_pixel(
                                    static_cast<std::uint32_t>(x),
                                    static_cast<std::uint32_t>(y),
                                    renderer2d::blend_mode_color(shadow, px, renderer2d::BlendMode::Normal));
                            }
                        }
                    }

                    for (int y = card_y; y < card_y + card_h; ++y) {
                        if (y < 0 || y >= static_cast<int>(state.height)) continue;
                        for (int x = card_x; x < card_x + card_w; ++x) {
                            if (x < 0 || x >= static_cast<int>(state.width)) continue;
                            const bool is_border = x == card_x || x == card_x + card_w - 1 || y == card_y || y == card_y + card_h - 1;
                            fallback_surface->set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
                                is_border ? border : fill);
                        }
                    }

                    multiply_surface_alpha(*fallback_surface, static_cast<float>(fallback_layer.opacity));
                    composite_surface(target_surface, *fallback_surface, 0, 0, BlendMode::Normal);
                }
                i = last_block_idx;
#else
                // 3D disabled: skip layer
#endif
                continue;
            }

            auto layer_surface = render_layer_surface(layer, state, intent, plan, task, render_context, tile_rect);

            // Apply mesh deformation (AE Puppet tool style) before effects
            if (layer.mesh_deform_id.has_value() && layer_surface) {
                auto it = intent.layer_resources.find(layer.id);
                if (it != intent.layer_resources.end() && it->second.mesh_deform) {
                    if (auto* mesh_ptr = dynamic_cast<const renderer2d::DeformMesh*>(it->second.mesh_deform.get())) {
                        layer_surface = apply_mesh_deform(
                            layer_surface,
                            *mesh_ptr,
                            layer.width,
                            layer.height);
                    }
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
                        effect_registry,
                        rendered_surfaces,
                        layer.id,
                        render_context.diagnostics);
                    record_timing(render_context.diagnostics, "adjustment", layer.id, adjustment_start);
                    
                    if (res.ok()) {
                        auto& adjusted = *res.value;
                        multiply_surface_alpha(adjusted, static_cast<float>(layer.opacity));
                        composite_surface(target_surface, adjusted, 0, 0, BlendMode::Normal);
                    } else {
                        // Error already recorded in diagnostics
                    }
                }
                continue;
            }

            // Effects
            if (render_context.policy.effects_enabled && (!layer.effects.empty() || !layer.animated_effects.empty())) {
                const auto effects_start = std::chrono::high_resolution_clock::now();
                std::vector<EffectSpec> resolved_effects = layer.effects;
                resolved_effects.reserve(resolved_effects.size() + layer.animated_effects.size());
                for (const auto& animated_effect : layer.animated_effects) {
                    resolved_effects.push_back(animated_effect.evaluate(layer.local_time_seconds));
                }
                auto res = apply_effect_pipeline(
                    *layer_surface,
                    resolved_effects,
                    host,
                    render_context.working_color_space.profile,
                    effect_registry,
                    rendered_surfaces,
                    layer.id,
                    render_context.diagnostics);
                record_timing(render_context.diagnostics, "effect_pipeline", layer.id, effects_start);
                
                if (res.ok()) {
                    *layer_surface = std::move(*res.value);
                } else {
                    // Effect failed, clear surface to prevent rendering stale data
                    layer_surface->clear(Color::transparent());
                }
            }

            // Layer Transitions - Unified transition pipeline
            if (render_context.policy.effects_enabled) {
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
                        resolution = resolve_layer_transition(layer.transition_in, render_context.transition_registry);
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
                        resolution = resolve_layer_transition(layer.transition_out, render_context.transition_registry);
                    }
                }

                // Apply transition if active
                if ((in_transition || out_transition) && transition_t > 0.0) {
                    if (resolution.valid && resolution.cpu_function != nullptr) {
                        // Use registry transition (pixel-level)
                        SurfaceRGBA transition_input(layer_surface->width(), layer_surface->height());
                        SurfaceRGBA transition_to(layer_surface->width(), layer_surface->height());

                        if (in_transition) {
                            // Transition in: from nothing (transparent) to layer
                            transition_input.clear(Color::transparent());
                            transition_to = *layer_surface;
                        } else {
                            // Transition out: from layer to nothing (transparent)
                            transition_input = *layer_surface;
                            transition_to.clear(Color::transparent());
                        }

                        // Apply transition pixel by pixel
                        SurfaceRGBA transition_result(layer_surface->width(), layer_surface->height());
                        const float layer_w = static_cast<float>(layer.width);
                        const float layer_h = static_cast<float>(layer.height);
                        const float offset_x = tile_rect ? static_cast<float>(tile_rect->x) : 0.0f;
                        const float offset_y = tile_rect ? static_cast<float>(tile_rect->y) : 0.0f;

                        #pragma omp parallel for schedule(static)
                        for (int y = 0; y < static_cast<int>(transition_result.height()); ++y) {
                            if (render_context.cancel_flag && render_context.cancel_flag->load()) continue;
                            for (std::uint32_t x = 0; x < transition_result.width(); ++x) {
                                // Use global UVs relative to the full layer, not the tile
                                const float u = (static_cast<float>(x) + offset_x + 0.5f) / layer_w;
                                const float v = (static_cast<float>(y) + offset_y + 0.5f) / layer_h;
                                const Color out = resolution.cpu_function(u, v, static_cast<float>(transition_t), transition_input, &transition_to);
                                transition_result.set_pixel(x, static_cast<std::uint32_t>(y), out);
                            }
                        }
                        *layer_surface = std::move(transition_result);
                    }
                    // Note: Basic transitions (fade, slide, zoom) are now handled via the registry
                    // as pixel-level transitions, unifying the pipeline
                }
            }

            // Particle effect
            /*
            // Particle effect
            if (render_context.policy.effects_enabled && layer.particle_spec.has_value()) {
                auto particle_effect_spec = particle_spec_to_effect_spec(*layer.particle_spec, layer.local_time_seconds);
                if (particle_effect_spec && particle_effect_spec->enabled) {
                    EffectParams params = effect_params_from_spec(*particle_effect_spec, render_context.cms.working_profile);
                    auto particle_surface = host.apply("tachyon.effect.generators.particle_emitter", *layer_surface, params);
                    *layer_surface = std::move(particle_surface);
                }
            }
            */

            // Opacity
            multiply_surface_alpha(*layer_surface, static_cast<float>(layer.opacity));

            // Apply resolved matte if available
            if (i < matte_buffers.size() && !matte_buffers[i].empty()) {
                ::tachyon::renderer2d::apply_matte_buffer(*layer_surface, matte_buffers[i], state.width, state.height);
            }

            // Final Composite
            float layer_inv_z = -1.0f;
            if (layer.world_position3.z != 0.0f) {
                // Approximate inv_z for 2D layer (this is a simplified mapping)
                // In a real perspective setup, this would be more complex.
                // Assuming Z is distance from camera in same units as 3D.
                float z = std::abs(layer.world_position3.z);
                if (z > 0.001f) {
                    layer_inv_z = 1.0f / z;
                }
            }

            composite_surface(target_surface, *layer_surface, 0, 0, parse_blend_mode(layer.blend_mode), layer_inv_z);
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
                && (!layer.effects.empty() || !layer.animated_effects.empty() || has_transition);
        });

    if (context.policy.tile_size > 0 && !has_effectful_layers) {
        TileGrid grid = build_tile_grid({0, 0, static_cast<int>(working_width), static_cast<int>(working_height)}, working_width, working_height, context.policy.tile_size);
        if (std::getenv("TACHYON_DIAGNOSTICS")) {
            std::cerr << "DIAG: Tiling " << working_width << "x" << working_height << " into " << grid.tiles.size() << " tiles (size " << context.policy.tile_size << ")\n";
        }
        for (int i = 0; i < static_cast<int>(grid.tiles.size()); ++i) {
            const auto& tile = grid.tiles[i];
            if (std::getenv("TACHYON_DIAGNOSTICS")) {
                std::cerr << "DIAG:   Tile " << i << ": {" << tile.x << "," << tile.y << "," << tile.width << "," << tile.height << "}\n";
            }
            SurfaceRGBA tile_surface(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
            tile_surface.clear(Color::transparent());
            
            RenderContext2D thread_context = context;
            render_pass(tile_surface, thread_context, tile);
            composite_surface(dst, tile_surface, tile.x, tile.y, BlendMode::Normal);
        }
    } else {
        render_pass(dst, context);
    }

    return frame;
}

} // namespace tachyon
