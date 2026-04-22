#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/runtime/execution/scheduling/tile_scheduler.h"
#include "tachyon/output/frame_aov.h"
#include <algorithm>
#include <cmath>

namespace tachyon {
using namespace renderer2d;

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
        return frame;
    }

    auto& dst = *frame.surface;
    dst.set_profile(context.cms.working_profile);
    dst.clear(Color::transparent());

    EffectHost& host = effect_host_for(context);

    // Identify if we have 3D layers and trigger 3D pass if available
    bool has_any_3d = std::any_of(state.layers.begin(), state.layers.end(), [](const auto& l) { return l.is_3d && l.visible; });
    if (has_any_3d && !context.ray_tracer) {
        context.ray_tracer = std::make_shared<renderer3d::RayTracer>();
    }

    auto render_pass = [&](SurfaceRGBA& target_surface, const std::optional<RectI>& tile_rect = std::nullopt) {
        // AE Rendering Order: Bottom to Top
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            const auto& layer = state.layers[i];
            if (!layer.enabled || !layer.active) {
                continue;
            }

            if (layer.is_3d && layer.visible) {
                if (tile_rect) {
                    continue; // Skip 3D blocks in tiled mode.
                }
                
                // Found a 3D block. Collect contiguous 3D layers.
                std::vector<std::size_t> block_indices;
                block_indices.push_back(i);
                std::size_t last_block_idx = i;
                for (std::size_t j = i + 1; j < state.layers.size(); ++j) {
                    if (state.layers[j].is_3d && state.layers[j].visible && state.layers[j].enabled && state.layers[j].active) {
                        block_indices.push_back(j);
                        last_block_idx = j;
                    } else if (state.layers[j].enabled && state.layers[j].active) {
                        break;
                    }
                }

                // Construct EvaluatedScene3D bridge from the composition subset
                renderer3d::EvaluatedScene3D scene3d;
                if (state.camera.available) {
                    scene3d.camera.position = state.camera.camera.position;
                    scene3d.camera.target = state.camera.camera.target;
                    scene3d.camera.up = state.camera.camera.up;
                    scene3d.camera.fov_y = state.camera.camera.fov;
                    
                    if (state.camera.previous_position) {
                        scene3d.camera.previous_position = *state.camera.previous_position;
                    } else {
                        scene3d.camera.previous_position = state.camera.camera.position;
                    }

                    if (state.camera.previous_point_of_interest) {
                        scene3d.camera.previous_target = *state.camera.previous_point_of_interest;
                    } else {
                        scene3d.camera.previous_target = state.camera.camera.target;
                    }
                }

                for (std::size_t idx : block_indices) {
                    const auto& l = state.layers[idx];
                    renderer3d::EvaluatedMeshInstance inst;
                    inst.object_id = static_cast<std::uint32_t>(idx);
                    inst.material_id = 0;
                    inst.world_transform = l.world_matrix;

                    if (l.previous_world_matrix) {
                        inst.previous_world_transform = *l.previous_world_matrix;
                    } else {
                        inst.previous_world_transform = inst.world_transform;
                    }
                    inst.material.base_color = l.fill_color;
                    inst.material.opacity = static_cast<float>(l.opacity);
                    // Assuming empty string triggers the generic quad fallback in RayTracer for now
                    inst.mesh_asset_id = l.asset_path.value_or("");
                    scene3d.instances.push_back(inst);
                }

                // Render the 3D block
                context.ray_tracer->set_samples_per_pixel(context.policy.ray_tracer_spp);
                context.ray_tracer->build_scene(scene3d);
                
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
                blur_config.curve = plan.motion_blur_curve;
                motion_blur.set_config(blur_config);

                const double frame_rate_value = state.frame_rate.value() > 0.0 ? state.frame_rate.value() : 60.0;
                const double frame_duration_seconds = 1.0 / frame_rate_value;

                context.ray_tracer->render(
                    scene3d,
                    aovs,
                    &motion_blur,
                    task.time_seconds,
                    frame_duration_seconds);
                
                auto world_3d = std::make_shared<SurfaceRGBA>(w, h);
                auto depth_aov_surf = std::make_shared<SurfaceRGBA>(w, h);
                auto normal_aov_surf = std::make_shared<SurfaceRGBA>(w, h);
                auto motion_vector_aov_surf = std::make_shared<SurfaceRGBA>(w, h);
                
                for (std::uint32_t y = 0; y < h; ++y) {
                    for (std::uint32_t x = 0; x < w; ++x) {
                        world_3d->set_pixel(x, y, aovs.beauty_rgba.at(x, y));
                        const float d = aovs.depth_z.at(x, y);
                        depth_aov_surf->set_pixel(x, y, {d, 0, 0, 1.0f});
                        
                        const math::Vector3 n = aovs.normal_xyz.at(x, y);
                        normal_aov_surf->set_pixel(x, y, {n.x, n.y, n.z, 1.0f});

                        const math::Vector2 mv = aovs.motion_vector_xy.at(x, y);
                        motion_vector_aov_surf->set_pixel(x, y, {mv.x, mv.y, 0.0f, 1.0f});
                    }
                }

                // Add to AOVs
                frame.aovs.push_back({"depth", depth_aov_surf});
                frame.aovs.push_back({"normal", normal_aov_surf});
                frame.aovs.push_back({"motion_vector", motion_vector_aov_surf});

                composite_surface(target_surface, *world_3d, 0, 0, BlendMode::Normal);
                i = last_block_idx;
                continue;
            }

            auto layer_surface = render_layer_surface(layer, state, plan, task, context, tile_rect);

            if (layer.is_adjustment_layer) {
                if (context.policy.effects_enabled) {
                    auto adjusted = apply_effect_pipeline(target_surface, layer.effects, host, context.working_color_space.profile);
                    multiply_surface_alpha(adjusted, static_cast<float>(layer.opacity));
                    composite_surface(target_surface, adjusted, 0, 0, BlendMode::Normal);
                }
                continue;
            }

            // Effects
            if (context.policy.effects_enabled && !layer.effects.empty()) {
                auto effect_surface = apply_effect_pipeline(*layer_surface, layer.effects, host, context.cms.working_profile);
                *layer_surface = std::move(effect_surface);
            }

            // Opacity
            multiply_surface_alpha(*layer_surface, static_cast<float>(layer.opacity));

            // Track Matte
            if (layer.track_matte_layer_index.has_value() && layer.track_matte_type != TrackMatteType::None) {
                const auto matte_idx = *layer.track_matte_layer_index;
                if (matte_idx < state.layers.size()) {
                    const auto& matte_layer = state.layers[matte_idx];
                    auto matte_surface = render_layer_surface(matte_layer, state, plan, task, context, tile_rect);
                    apply_mask(*layer_surface, *matte_surface, layer.track_matte_type);
                }
            }

            // Final Composite
            int ox = 0, oy = 0;
            composite_surface(target_surface, *layer_surface, ox, oy, parse_blend_mode(layer.blend_mode));
        }
    };

    if (context.policy.tile_size > 0) {
        TileGrid grid = build_tile_grid({0, 0, static_cast<int>(working_width), static_cast<int>(working_height)}, working_width, working_height, context.policy.tile_size);
        
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < static_cast<int>(grid.tiles.size()); ++i) {
            const auto& tile = grid.tiles[i];
            SurfaceRGBA tile_surface(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
            tile_surface.clear(Color::transparent());
            
            RenderContext2D thread_context = context;
            render_pass(tile_surface, tile);
            
            #pragma omp critical
            {
                dst.blit(tile_surface, tile.x, tile.y);
            }
        }
    } else {
        render_pass(dst);
    }

    return frame;
}

} // namespace tachyon
