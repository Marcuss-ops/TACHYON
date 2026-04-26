#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"

#ifdef _MSC_VER
#pragma warning(disable:4456)
#endif
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/renderer2d_matte_resolver.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/text_mesh_builder.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/media_card_mesh_builder.h"
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/effects/depth_of_field.h"
#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/runtime/execution/scheduling/tile_scheduler.h"
#include "tachyon/output/frame_aov.h"
#include <algorithm>
#include <cmath>
#include <array>
#include <filesystem>
#include <unordered_map>

namespace tachyon {
using namespace renderer2d;

namespace {

std::optional<EffectSpec> particle_spec_to_effect_spec(const ParticleSpec& spec, double time_seconds) {
    EffectSpec effect;
    effect.type = "particle_emitter";
    effect.enabled = true;
    const double emission_rate = spec.emission_rate.value.value_or(100.0);
    const double lifetime = spec.lifetime.value.value_or(2.0);
    const double count = std::max(1.0, emission_rate * lifetime);
    effect.scalars["seed"] = static_cast<double>(static_cast<std::uint64_t>(time_seconds * 1000) % 1000000ULL);
    effect.scalars["count"] = count;
    effect.scalars["time"] = time_seconds;
    effect.scalars["lifetime"] = lifetime;
    effect.scalars["speed"] = spec.start_speed.value.value_or(100.0);
    effect.scalars["gravity"] = spec.gravity.value.value_or(980.0);
    effect.scalars["spread_x"] = 1.0;
    effect.scalars["spread_y"] = 1.0;
    const double start_s = spec.start_size.value.value_or(5.0);
    const double end_s = spec.end_size.value.value_or(0.0);
    effect.scalars["radius_min"] = std::min(start_s, end_s);
    effect.scalars["radius_max"] = std::max(start_s, end_s);
    effect.colors["color"] = spec.start_color.value.value_or(ColorSpec{255, 255, 255, 255});
    effect.scalars["opacity"] = 1.0;
    effect.scalars["center_x"] = 0.5;
    effect.scalars["center_y"] = 0.5;
    effect.scalars["emit_width"] = 1.0;
    effect.scalars["emit_height"] = 1.0;
    return effect;
}

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

    for (const auto& reference : references) {
        if (reference.empty()) {
            continue;
        }

        std::filesystem::path resolved = context.media_manager->get_asset_path(reference);
        if (!resolved.empty()) {
            return resolved;
        }

        std::filesystem::path candidate(reference);
        if (candidate.has_extension()) {
            return candidate;
        }
    }

    return std::nullopt;
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context) {

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

    auto& dst = *frame.surface;
    dst.set_profile(context.cms.working_profile);
    dst.clear(Color::transparent());

    // Identify if we have 3D layers and trigger 3D pass if available
    bool has_any_3d = std::any_of(state.layers.begin(), state.layers.end(), [](const auto& l) { return l.is_3d && l.visible; });
    if (has_any_3d && !context.ray_tracer) {
        context.ray_tracer = std::make_shared<renderer3d::RayTracer>(context.media_manager);
    }

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
        const auto& layer = state.layers[i];
        if (!layer.enabled || !layer.active || layer.id.empty()) {
            continue;
        }

        // Skip 3D layers for now (they're handled separately)
        if (layer.is_3d && layer.visible) {
            continue;
        }

        auto layer_surface = render_layer_surface(layer, state, plan, task, context, std::nullopt);
        if (layer_surface) {
            rendered_surfaces[layer.id] = layer_surface;
        }
    }

    // Resolve matte dependencies using MatteResolver
    Renderer2DMatteResolver resolver;
    std::string validation_error;
    if (!matte_dependencies.empty() && !resolver.validate(state.layers, matte_dependencies, validation_error)) {
        frame.note += " [matte validation warning: " + validation_error + "]";
    }

    std::vector<std::vector<float>> matte_buffers;
    if (!matte_dependencies.empty()) {
        resolver.resolve(state.layers, matte_dependencies, rendered_surfaces, matte_buffers, state.width, state.height);
    }

    auto render_pass = [&](SurfaceRGBA& target_surface, RenderContext2D& render_context, const std::optional<RectI>& tile_rect = std::nullopt) {
        EffectHost& host = effect_host_for(render_context);
        // Render layers in stack order so higher layers can affect the composite below them.
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            const auto& layer = state.layers[i];
            if (!layer.enabled || !layer.active) {
                continue;
            }

            if (layer.is_3d && layer.visible) {
                if (tile_rect) {
                    continue; // Skip 3D blocks in tiled mode.
                }

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
                renderer3d::EvaluatedScene3D scene3d;
                if (state.camera.available) {
                    scene3d.camera.position = state.camera.position;
                    scene3d.camera.target = state.camera.point_of_interest;
                    scene3d.camera.up = {0.0f, 1.0f, 0.0f};
                    scene3d.camera.fov_y = state.camera.angle_of_view;
                    scene3d.camera.focal_length_mm = state.camera.focal_length;
                    scene3d.camera.focal_distance = state.camera.focus_distance;
                    scene3d.camera.aperture = state.camera.aperture;
                    scene3d.camera.previous_position = state.camera.position;
                    scene3d.camera.previous_target = state.camera.point_of_interest;
                }

                for (std::size_t block_idx : block_indices) {
                    const auto& l = state.layers[block_idx];
                    renderer3d::EvaluatedMeshInstance inst;
                    inst.object_id = static_cast<std::uint32_t>(block_idx);
                    inst.material_id = 0;
                    inst.world_transform = l.world_matrix;
                    inst.previous_world_transform = l.previous_world_matrix;
                    inst.material.base_color = l.fill_color;
                    inst.material.opacity = static_cast<float>(l.opacity);
                    if (l.type == scene::LayerType::Text && render_context.font_registry != nullptr) {
                        ::tachyon::text::TextAnimationOptions animation{};
                        animation.time_seconds = static_cast<float>(l.local_time_seconds);
                        animation.animators = std::span<const ::tachyon::TextAnimatorSpec>(l.text_animators.data(), l.text_animators.size());
                        const auto text_mesh = build_text_extrusion_mesh(l, state, *render_context.font_registry, animation);
                        if (text_mesh.mesh) {
                            inst.mesh_asset = text_mesh.mesh;
                            inst.mesh_asset_id = text_mesh.cache_key;
                        }
                    }
                    if (!inst.mesh_asset) {
                        if (l.type == scene::LayerType::Image || l.type == scene::LayerType::Video) {
                            const auto media_source = resolve_media_source(l, render_context);
                            if (media_source.has_value()) {
                                const ::tachyon::renderer2d::SurfaceRGBA* media_frame = nullptr;
                                if (l.type == scene::LayerType::Video) {
                                    media_frame = render_context.media_manager->get_video_frame(*media_source, task.time_seconds);
                                } else {
                                    media_frame = render_context.media_manager->get_image(*media_source);
                                }

                                if (media_frame != nullptr) {
                                    const auto media_mesh = build_textured_card_mesh(
                                        l,
                                        *media_frame,
                                        media_source->generic_string() + "@" + std::to_string(task.time_seconds));
                                    inst.mesh_asset = media_mesh.mesh;
                                    inst.mesh_asset_id = media_mesh.cache_key;
                                } else {
                                    const auto fallback_mesh = build_colored_card_mesh(
                                        l,
                                        media_source->generic_string());
                                    inst.mesh_asset = fallback_mesh.mesh;
                                    inst.mesh_asset_id = fallback_mesh.cache_key;
                                }
                            }
                        } else if (l.type == scene::LayerType::Solid) {
                            const auto solid_mesh = build_colored_card_mesh(l, std::to_string(task.frame_number));
                            inst.mesh_asset = solid_mesh.mesh;
                            inst.mesh_asset_id = solid_mesh.cache_key;
                        }
                    }

                    if (inst.mesh_asset_id.empty()) {
                        // Fallback to a registered mesh asset if the layer already points to one.
                        inst.mesh_asset_id = l.asset_path.value_or("");
                    }
                    scene3d.instances.push_back(inst);
                }

                // Render the 3D block
                render_context.ray_tracer->set_samples_per_pixel(render_context.policy.ray_tracer_spp);
                render_context.ray_tracer->set_denoiser_enabled(render_context.policy.denoiser_enabled);
                render_context.ray_tracer->build_scene(scene3d);

                const std::uint32_t w = static_cast<std::uint32_t>(state.width);
                const std::uint32_t h = static_cast<std::uint32_t>(state.height);

                renderer3d::AOVBuffer aovs;
                aovs.resize(w, h);
                aovs.clear();

                renderer3d::MotionBlurRenderer motion_blur;
                renderer3d::MotionBlurRenderer::MotionBlurConfig blur_config;
                blur_config.enabled = plan.motion_blur_enabled;
                blur_config.samples = static_cast<int>(std::max<std::int64_t>(1, plan.motion_blur_samples));
                blur_config.shutter_angle = plan.motion_blur_shutter_angle;
                blur_config.shutter_phase = plan.motion_blur_shutter_phase;
                // Convert string to MotionBlurWeightCurve enum
                if (plan.motion_blur_curve == "triangle") {
                    blur_config.weight_curve = renderer3d::MotionBlurRenderer::MotionBlurWeightCurve::kTriangle;
                } else if (plan.motion_blur_curve == "gaussian") {
                    blur_config.weight_curve = renderer3d::MotionBlurRenderer::MotionBlurWeightCurve::kGaussian;
                } else {
                    blur_config.weight_curve = renderer3d::MotionBlurRenderer::MotionBlurWeightCurve::kBox;
                }
                motion_blur.set_config(blur_config);

                const double frame_rate_value = state.frame_rate.value() > 0.0 ? state.frame_rate.value() : 60.0;
                const double frame_duration_seconds = 1.0 / frame_rate_value;

                render_context.ray_tracer->render(
                    scene3d,
                    aovs,
                    &motion_blur,
                    task.time_seconds,
                    frame_duration_seconds);

                if (plan.dof.enabled || render_context.policy.dof_sample_count > 1) {
                    renderer3d::DepthOfFieldConfig dof_config = renderer3d::DepthOfFieldConfig::from_quality_policy(
                        render_context.policy,
                        scene3d.camera.focal_distance,
                        scene3d.camera.aperture,
                        scene3d.camera.focal_length_mm,
                        36.0f);
                    dof_config.enabled = plan.dof.enabled || render_context.policy.dof_sample_count > 1;
                    if (plan.dof.aperture > 0.0) {
                        dof_config.aperture_fstop = static_cast<float>(plan.dof.aperture);
                    }
                    if (plan.dof.focus_distance > 0.0) {
                        dof_config.focal_distance = static_cast<float>(plan.dof.focus_distance);
                    }
                    if (plan.dof.focal_length > 0.0) {
                        dof_config.focal_length_mm = static_cast<float>(plan.dof.focal_length);
                    }
                    renderer3d::DepthOfFieldPostPass dof_pass(dof_config);
                    const math::Vector3 camera_forward = (scene3d.camera.target - scene3d.camera.position).normalized();
                    dof_pass.apply(aovs, scene3d.camera.position, camera_forward);
                }

                auto world_3d = std::make_shared<SurfaceRGBA>(w, h);
                auto depth_aov_surf = std::make_shared<SurfaceRGBA>(w, h);
                auto normal_aov_surf = std::make_shared<SurfaceRGBA>(w, h);
                auto motion_vector_aov_surf = std::make_shared<SurfaceRGBA>(w, h);

                for (std::uint32_t y = 0; y < h; ++y) {
                    for (std::uint32_t x = 0; x < w; ++x) {
                        const std::size_t pixel_index = (static_cast<std::size_t>(y) * w + x) * 4;
                        world_3d->set_pixel(x, y, {
                            aovs.beauty_rgba[pixel_index + 0],
                            aovs.beauty_rgba[pixel_index + 1],
                            aovs.beauty_rgba[pixel_index + 2],
                            aovs.beauty_rgba[pixel_index + 3]
                        });

                        const float d = aovs.depth_z[static_cast<std::size_t>(y) * w + x];
                        depth_aov_surf->set_pixel(x, y, {d, 0, 0, 1.0f});

                        const std::size_t normal_index = (static_cast<std::size_t>(y) * w + x) * 3;
                        const math::Vector3 n{
                            aovs.normal_xyz[normal_index + 0],
                            aovs.normal_xyz[normal_index + 1],
                            aovs.normal_xyz[normal_index + 2]
                        };
                        normal_aov_surf->set_pixel(x, y, {n.x, n.y, n.z, 1.0f});

                        const std::size_t mv_index = (static_cast<std::size_t>(y) * w + x) * 2;
                        const math::Vector2 mv{
                            aovs.motion_vector_xy[mv_index + 0],
                            aovs.motion_vector_xy[mv_index + 1]
                        };
                        motion_vector_aov_surf->set_pixel(x, y, {mv.x, mv.y, 0.0f, 1.0f});
                    }
                }

                // Add to AOVs
                frame.aovs.push_back({"depth", depth_aov_surf});
                frame.aovs.push_back({"normal", normal_aov_surf});
                frame.aovs.push_back({"motion_vector", motion_vector_aov_surf});

                {
                    auto layer_bounds = layer_rect(layer, static_cast<std::int64_t>(state.width), static_cast<std::int64_t>(state.height), res_scale);
                    composite_surface(target_surface, *world_3d, layer_bounds.x, layer_bounds.y, BlendMode::Normal);
                }
                i = last_block_idx;
                continue;
            }

            auto layer_surface = render_layer_surface(layer, state, plan, task, render_context, tile_rect);

            if (layer.is_adjustment_layer) {
                if (render_context.policy.effects_enabled) {
                    auto adjusted = apply_effect_pipeline(
                        target_surface,
                        layer.effects,
                        layer.animated_effects,
                        layer.local_time_seconds,
                        host,
                        render_context.working_color_space.profile,
                        rendered_surfaces,
                        layer.id);
                    multiply_surface_alpha(adjusted, static_cast<float>(layer.opacity));
                    composite_surface(target_surface, adjusted, 0, 0, BlendMode::Normal);
                }
                continue;
            }

            // Effects
            if (render_context.policy.effects_enabled && (!layer.effects.empty() || !layer.animated_effects.empty())) {
                auto effect_surface = apply_effect_pipeline(
                    *layer_surface,
                    layer.effects,
                    layer.animated_effects,
                    layer.local_time_seconds,
                    host,
                    render_context.cms.working_profile,
                    rendered_surfaces,
                    layer.id);
                *layer_surface = std::move(effect_surface);
            }

            // Particle effect
            if (render_context.policy.effects_enabled && layer.particle_spec.has_value()) {
                auto particle_effect_spec = particle_spec_to_effect_spec(*layer.particle_spec, layer.local_time_seconds);
                if (particle_effect_spec && particle_effect_spec->enabled) {
                    EffectParams params = effect_params_from_spec(*particle_effect_spec, render_context.cms.working_profile);
                    auto particle_surface = host.apply("particle_emitter", *layer_surface, params);
                    *layer_surface = std::move(particle_surface);
                }
            }

            // Opacity
            multiply_surface_alpha(*layer_surface, static_cast<float>(layer.opacity));

            // Apply resolved matte if available
            if (i < matte_buffers.size() && !matte_buffers[i].empty()) {
                ::tachyon::renderer2d::apply_matte_buffer(*layer_surface, matte_buffers[i], state.width, state.height);
            }

            // Final Composite
            auto layer_bounds = layer_rect(layer, static_cast<std::int64_t>(state.width), static_cast<std::int64_t>(state.height), res_scale);
            composite_surface(target_surface, *layer_surface, layer_bounds.x, layer_bounds.y, parse_blend_mode(layer.blend_mode));
        }
    };

    if (context.policy.tile_size > 0) {
        TileGrid grid = build_tile_grid({0, 0, static_cast<int>(working_width), static_cast<int>(working_height)}, working_width, working_height, context.policy.tile_size);
        
        for (int i = 0; i < static_cast<int>(grid.tiles.size()); ++i) {
            const auto& tile = grid.tiles[i];
            SurfaceRGBA tile_surface(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
            tile_surface.clear(Color::transparent());
            
            RenderContext2D thread_context = context;
            render_pass(tile_surface, thread_context, tile);
            dst.blit(tile_surface, tile.x, tile.y);
        }
    } else {
        render_pass(dst, context);
    }

    return frame;
}

} // namespace tachyon
