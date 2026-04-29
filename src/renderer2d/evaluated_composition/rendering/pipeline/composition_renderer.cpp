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
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.h"
#include "tachyon/renderer2d/raster/mesh_rasterizer.h"
#include "tachyon/renderer2d/raster/mesh_deform_apply.h"
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/effects/depth_of_field.h"
#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/renderer3d/visibility/culling.h"
#include "tachyon/runtime/execution/scheduling/tile_scheduler.h"
#include "tachyon/output/frame_aov.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/core/animation/easing.h"
#include <algorithm>
#include <cmath>
#include <array>
#include <filesystem>
#include <unordered_map>

namespace tachyon {

namespace {

renderer2d::Color background_spec_to_color(const BackgroundSpec& bg) {
    if (bg.is_color() && bg.parsed_color.has_value()) {
        const auto& c = *bg.parsed_color;
        return renderer2d::Color{
            static_cast<float>(c.r) / 255.0f,
            static_cast<float>(c.g) / 255.0f,
            static_cast<float>(c.b) / 255.0f,
            static_cast<float>(c.a) / 255.0f
        };
    }
    return renderer2d::Color::transparent();
}

} // namespace
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
    
    renderer2d::Color clear_color = renderer2d::Color::transparent();
    if (plan.composition.background.has_value()) {
        clear_color = background_spec_to_color(*plan.composition.background);
    }
    dst.clear(clear_color);

    // Identify if we have 3D layers and trigger 3D pass if available
    bool has_any_3d = std::any_of(state.layers.begin(), state.layers.end(), [](const auto& l) { return l.is_3d && l.visible; });
    if (has_any_3d && !context.ray_tracer) {
        context.ray_tracer = std::make_shared<renderer3d::RayTracer>(context.media_manager);
    }

    std::vector<bool> visible_3d_layers(state.layers.size(), true);
    if (has_any_3d && state.camera.available) {
        renderer3d::SceneCulling culling;
        culling.set_camera(state.camera.camera, state.camera.position);
        const auto culling_result = culling.cull_layers(state);
        visible_3d_layers.assign(state.layers.size(), false);
        for (const auto index : culling_result.visibleIndices) {
            if (index < visible_3d_layers.size()) {
                visible_3d_layers[index] = true;
            }
        }
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
        
        // Apply mesh deformation for matte resolution
        if (layer.mesh_deform_enabled && layer.mesh_deform && layer_surface) {
            layer_surface = apply_mesh_deform(
                layer_surface,
                *layer.mesh_deform,
                layer.width,
                layer.height);
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
                    scene3d.camera.previous_position = state.camera.previous_position;
                    scene3d.camera.previous_target = state.camera.previous_point_of_interest;
                    scene3d.camera.previous_up = scene3d.camera.up;
                    scene3d.camera.camera_id = state.camera.layer_id;
                    scene3d.camera.is_active_camera = true;
                }

                auto to_color = [](const math::Vector3& color) -> ColorSpec {
                    const auto clamp_channel = [](float v) -> std::uint8_t {
                        const float clamped = std::clamp(v, 0.0f, 1.0f);
                        return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
                    };
                    return ColorSpec{
                        clamp_channel(color.x),
                        clamp_channel(color.y),
                        clamp_channel(color.z),
                        255
                    };
                };

                scene3d.lights.reserve(state.lights.size());
                for (const auto& light_state : state.lights) {
                    renderer3d::EvaluatedLight light3d;
                    if (light_state.type == "ambient") {
                        light3d.type = renderer3d::LightType::Ambient;
                    } else if (light_state.type == "parallel") {
                        light3d.type = renderer3d::LightType::Directional;
                    } else if (light_state.type == "spot") {
                        light3d.type = renderer3d::LightType::Spot;
                    } else {
                        light3d.type = renderer3d::LightType::Point;
                    }
                    light3d.position = light_state.position;
                    light3d.direction = light_state.direction;
                    light3d.color = to_color(light_state.color);
                    light3d.intensity = light_state.intensity;
                    light3d.attenuation_radius = light_state.attenuation_far > 0.0f ? light_state.attenuation_far : light_state.attenuation_near;
                    light3d.spot_angle = light_state.cone_angle;
                    light3d.spot_penumbra = light_state.cone_feather;
                    light3d.casts_shadows = light_state.casts_shadows;
                    scene3d.lights.push_back(light3d);
                }

                if (plan.scene_spec != nullptr) {
                    const auto comp_it = std::find_if(
                        plan.scene_spec->compositions.begin(),
                        plan.scene_spec->compositions.end(),
                        [&](const CompositionSpec& comp) { return comp.id == plan.composition_target; });
                    if (comp_it != plan.scene_spec->compositions.end() && comp_it->environment_path.has_value()) {
                        scene3d.environment_map_id = *comp_it->environment_path;
                    }
                }

                for (std::size_t block_idx : block_indices) {
                    const auto& l = state.layers[block_idx];
                    if (block_idx >= visible_3d_layers.size() || !visible_3d_layers[block_idx]) {
                        continue;
                    }
                    renderer3d::EvaluatedMeshInstance inst;
                    inst.object_id = static_cast<std::uint32_t>(block_idx);
                    inst.material_id = 0;
                    inst.world_transform = l.world_matrix;
                    inst.previous_world_transform = l.previous_world_matrix;
                    inst.material.base_color = l.fill_color;
                    inst.material.opacity = static_cast<float>(l.opacity);
                    inst.material.metallic = l.material.metallic;
                    inst.material.roughness = std::max(0.05f, l.material.roughness);
                    inst.material.emission_strength = l.material.emission;
                    inst.material.emission_color = l.fill_color;
                    inst.material.transmission = l.material.transmission;
                    inst.material.ior = l.material.ior;
                    inst.joint_matrices = l.joint_matrices;
                    inst.morph_weights = l.morph_weights;
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

                if (scene3d.instances.empty()) {
                    i = last_block_idx;
                    continue;
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

                const bool world_has_visible_pixels = std::any_of(
                    aovs.beauty_rgba.begin() + 3,
                    aovs.beauty_rgba.end(),
                    [](float alpha) { return alpha > 0.001f; });

                // Add to AOVs
                frame.aovs.push_back({"depth", depth_aov_surf});
                frame.aovs.push_back({"normal", normal_aov_surf});
                frame.aovs.push_back({"motion_vector", motion_vector_aov_surf});

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
                            && candidate.type != scene::LayerType::UIHook
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
                continue;
            }

            auto layer_surface = render_layer_surface(layer, state, plan, task, render_context, tile_rect);

            // Apply mesh deformation (AE Puppet tool style) before effects
            if (layer.mesh_deform_enabled && layer.mesh_deform && layer_surface) {
                layer_surface = apply_mesh_deform(
                    layer_surface,
                    *layer.mesh_deform,
                    layer.width,
                    layer.height);
            }

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

            // Layer Transitions - Unified transition pipeline
            if (render_context.policy.effects_enabled) {
                init_builtin_transitions();
                const double layer_time = task.time_seconds;
                bool in_transition = false;
                bool out_transition = false;
                double transition_t = 0.0;
                const TransitionSpec* transition_spec = nullptr;

                // Check transition_in
                if (!layer.transition_in.transition_id.empty() || layer.transition_in.type != "none") {
                    const double relative_time = layer_time - layer.in_time;
                    const double transition_duration = layer.transition_in.duration;
                    const double start_time = layer.transition_in.delay;

                    if (relative_time >= start_time && relative_time < start_time + transition_duration) {
                        in_transition = true;
                        transition_t = (relative_time - start_time) / transition_duration;
                        transition_t = std::clamp(transition_t, 0.0, 1.0);
                        transition_t = animation::apply_easing(transition_t, layer.transition_in.easing, {}, layer.transition_in.spring);

                        // Unified lookup: transition_id first, then type as fallback
                        if (!layer.transition_in.transition_id.empty()) {
                            transition_spec = TransitionRegistry::instance().find(layer.transition_in.transition_id);
                        } else if (layer.transition_in.type != "none") {
                            // Look up type in registry (base transitions are now registered)
                            transition_spec = TransitionRegistry::instance().find(layer.transition_in.type);
                        }
                    }
                }

                // Check transition_out
                if (!in_transition && (!layer.transition_out.transition_id.empty() || layer.transition_out.type != "none")) {
                    const double time_until_end = layer.out_time - layer_time;
                    const double transition_duration = layer.transition_out.duration;

                    if (time_until_end >= 0.0 && time_until_end < transition_duration) {
                        out_transition = true;
                        transition_t = time_until_end / transition_duration;
                        transition_t = std::clamp(transition_t, 0.0, 1.0);
                        transition_t = animation::apply_easing(transition_t, layer.transition_out.easing, {}, layer.transition_out.spring);

                        // Unified lookup: transition_id first, then type as fallback
                        if (!layer.transition_out.transition_id.empty()) {
                            transition_spec = TransitionRegistry::instance().find(layer.transition_out.transition_id);
                        } else if (layer.transition_out.type != "none") {
                            // Look up type in registry (base transitions are now registered)
                            transition_spec = TransitionRegistry::instance().find(layer.transition_out.type);
                        }
                    }
                }

                // Apply transition if active
                if ((in_transition || out_transition) && transition_t > 0.0) {
                    if (transition_spec != nullptr && transition_spec->function != nullptr) {
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
                        const float width = static_cast<float>(std::max<std::uint32_t>(1U, transition_input.width()));
                        const float height = static_cast<float>(std::max<std::uint32_t>(1U, transition_input.height()));

                        for (std::uint32_t y = 0; y < transition_result.height(); ++y) {
                            for (std::uint32_t x = 0; x < transition_result.width(); ++x) {
                                const float u = (static_cast<float>(x) + 0.5f) / width;
                                const float v = (static_cast<float>(y) + 0.5f) / height;
                                const Color out = transition_spec->function(u, v, static_cast<float>(transition_t), transition_input, &transition_to);
                                transition_result.set_pixel(x, y, out);
                            }
                        }
                        *layer_surface = std::move(transition_result);
                    }
                    // Note: Basic transitions (fade, slide, zoom) are now handled via the registry
                    // as pixel-level transitions, unifying the pipeline
                }
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
            composite_surface(target_surface, *layer_surface, 0, 0, parse_blend_mode(layer.blend_mode));
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
