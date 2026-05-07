#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/pipeline_helpers.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/property_adapter.h"
#include "tachyon/core/animation/property_interpolation.h"
#include <map>
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>
#include <iostream>

namespace tachyon {

namespace {

struct LayerMeshResult {
    std::shared_ptr<const media::MeshAsset> mesh_asset;
    std::string mesh_asset_id;
};

static bool is_bridge_renderable_layer(const scene::EvaluatedLayerState& layer) {
    if (!layer.is_3d || !layer.visible || !layer.enabled || !layer.active) {
        return false;
    }

    switch (layer.type) {
        case LayerType::Camera:
        case LayerType::Light:
        case LayerType::NullLayer:
            return false;
        default:
            return true;
    }
}

static LayerSurface build_layer_surface_impl(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    LayerSurface result;

    if (input.rendered_surfaces) {
        auto it = input.rendered_surfaces->find(l.id);
        if (it != input.rendered_surfaces->end() && it->second) {
            result.surface = it->second;
            result.cache_key = l.id + ":" + std::to_string(input.task ? input.task->frame_number : 0);
            result.source_kind = "surface";
            return result;
        }
    }

    if (!input.context || !input.state) {
        return result;
    }

    std::shared_ptr<renderer2d::SurfaceRGBA> rendered;
    if (input.plan && input.task) {
        rendered = renderer2d::render_layer_surface(
            l,
            *input.state,
            *input.plan,
            *input.task,
            const_cast<renderer2d::RenderContext2D&>(*input.context),
            std::nullopt);
    } else {
        rendered = renderer2d::render_simple_layer_surface(
            l,
            *input.state,
            const_cast<renderer2d::RenderContext2D&>(*input.context),
            std::nullopt);
    }
    if (rendered) {
        result.surface = std::move(rendered);
        result.cache_key = l.id + ":" + std::to_string(input.task ? input.task->frame_number : 0);
        result.source_kind = "surface";
    }

    return result;
}

static LayerMeshResult build_layer_mesh(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    LayerMeshResult result;

    const auto layer_surface = build_layer_surface_impl(l, input);

    if (layer_surface.valid()) {
        const auto textured_mesh = renderer2d::build_textured_card_mesh(
            l,
            *layer_surface.surface,
            layer_surface.cache_key.empty() ? l.id : layer_surface.cache_key);
        result.mesh_asset = textured_mesh.mesh;
        result.mesh_asset_id = textured_mesh.cache_key;
        return result;
    }

    if (input.intent) {
        auto it = input.intent->layer_resources.find(l.id);
        if (it != input.intent->layer_resources.end() && it->second.mesh_asset) {
            result.mesh_asset = it->second.mesh_asset;
            result.mesh_asset_id = result.mesh_asset->path;
            if (result.mesh_asset_id.empty()) {
                result.mesh_asset_id = l.asset_path.value_or("");
            }
            return result;
        }
    }

    if (l.width > 0 && l.height > 0) {
        const auto solid_mesh = renderer2d::build_colored_card_mesh(l, std::to_string(input.task ? input.task->frame_number : 0));
        result.mesh_asset = solid_mesh.mesh;
        result.mesh_asset_id = solid_mesh.cache_key;
    }

    return result;
}

static EvaluatedMeshInstance3D build_instance_for_layer(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    EvaluatedMeshInstance3D inst;
    inst.object_id = 0; 
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

#ifdef TACHYON_ENABLE_3D
    const auto mesh_result = build_layer_mesh(l, input);
    inst.mesh_asset = mesh_result.mesh_asset;
    inst.mesh_asset_id = mesh_result.mesh_asset_id;

    if (l.three_d && l.three_d->enabled) {
        for (const auto& mod_spec : l.three_d->modifiers) {
            ResolvedModifier3D resolved;
            resolved.type = mod_spec.type;
            
            for (const auto& [name, anim] : mod_spec.scalar_params) {
                if (!anim.keyframes.empty()) {
                    auto generic_prop = animation::to_generic(anim);
                    resolved.scalar_params[name] = animation::sample_keyframes(
                        generic_prop.keyframes,
                        anim.value.value_or(0.0),
                        l.local_time_seconds,
                        animation::lerp_scalar
                    );
                } else if (anim.value.has_value()) {
                    resolved.scalar_params[name] = static_cast<float>(anim.value.value());
                }
            }
            
            for (const auto& [name, anim] : mod_spec.vector3_params) {
                if (anim.value.has_value()) {
                    resolved.vector3_params[name] = anim.value.value();
                }
            }
            
            resolved.string_params = mod_spec.string_params;
            inst.modifiers.push_back(std::move(resolved));
        }
    }
    
    if (!inst.modifiers.empty()) {
        inst.mesh_asset_id += "_mod_" + std::to_string(inst.modifiers.size());
    }
#endif

    if (inst.mesh_asset_id.empty()) {
        inst.mesh_asset_id = l.asset_path.value_or("");
    }

    return inst;
}

static ColorSpec to_color(const math::Vector3& color) {
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
}

} // anonymous namespace

EvaluatedCamera3D build_camera_3d(
    const scene::EvaluatedCameraState& camera,
    const scene::EvaluatedCompositionState& /*state*/) {
    EvaluatedCamera3D cam;
    
#ifdef TACHYON_ENABLE_3D
    cam.position = camera.position;
    cam.target = camera.point_of_interest;
    cam.up = camera.up;
    cam.fov_y = camera.fov_y_rad * 180.0f / 3.14159f; 
    cam.focal_length_mm = camera.camera.focal_length_mm;
    cam.focal_distance = camera.focus_distance;
    cam.aperture = camera.aperture;
    cam.camera_id = camera.layer_id;
    cam.is_active_camera = camera.available;

    math::Transform3 prev_transform = camera.previous_world_matrix.to_transform();
    cam.previous_position = prev_transform.position;
    cam.previous_target = camera.point_of_interest; 
    cam.previous_up = camera.up;

    cam.blur_level = camera.blur_level;
    cam.dof_enabled = camera.dof_enabled;
#endif

    return cam;
}

std::vector<EvaluatedLight3D> build_lights_3d(
    const std::vector<scene::EvaluatedLightState>& lights) {
    std::vector<EvaluatedLight3D> lights3d;
#ifdef TACHYON_ENABLE_3D
    lights3d.reserve(lights.size());

    for (const auto& light_state : lights) {
        EvaluatedLight3D light3d;
        if (light_state.type == "ambient") {
            light3d.type = LightType3D::Ambient;
        } else if (light_state.type == "parallel") {
            light3d.type = LightType3D::Directional;
        } else if (light_state.type == "spot") {
            light3d.type = LightType3D::Spot;
        } else {
            light3d.type = LightType3D::Point;
        }
        light3d.position = light_state.position;
        light3d.direction = light_state.direction;
        light3d.color = to_color(light_state.color);
        light3d.intensity = light_state.intensity;
        light3d.attenuation_radius = light_state.attenuation_far > 0.0f ? light_state.attenuation_far : light_state.attenuation_near;
        light3d.spot_angle = light_state.cone_angle;
        light3d.spot_penumbra = light_state.cone_feather;
        light3d.casts_shadows = light_state.casts_shadows;
        lights3d.push_back(light3d);
    }
#endif
    return lights3d;
}

std::vector<EvaluatedMeshInstance3D> build_instances_3d(
    const Scene3DBridgeInput& input) {
    std::vector<EvaluatedMeshInstance3D> instances;
#ifdef TACHYON_ENABLE_3D
    std::size_t instance_counter = 0;

    for (std::size_t block_idx : *input.block_indices) {
        if (block_idx >= input.state->layers.size()) {
            continue;
        }
        const auto& l = input.state->layers[block_idx];
        if (block_idx >= input.visible_3d_layers->size() || !(*input.visible_3d_layers)[block_idx]) {
            continue;
        }

        if (l.type == LayerType::Precomp && l.collapse_transformations && l.nested_composition) {
            for (const auto& nested_layer : l.nested_composition->layers) {
                if (!is_bridge_renderable_layer(nested_layer)) {
                    continue;
                }
                scene::EvaluatedLayerState adjusted = nested_layer;
                adjusted.world_matrix = l.world_matrix * nested_layer.world_matrix;
                adjusted.previous_world_matrix = l.previous_world_matrix * nested_layer.previous_world_matrix;

                auto inst = build_instance_for_layer(adjusted, input);
                inst.object_id = static_cast<std::uint32_t>(instance_counter++);
                instances.push_back(inst);
            }
            continue;
        }

        if (!is_bridge_renderable_layer(l)) {
            continue;
        }

        auto inst = build_instance_for_layer(l, input);
        inst.object_id = static_cast<std::uint32_t>(block_idx);
        instances.push_back(inst);
        instance_counter++;
    }
#endif
    return instances;
}

LayerSurface build_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const Scene3DBridgeInput& input) {
    return build_layer_surface_impl(layer, input);
}

Scene3DBridgeOutput build_evaluated_scene_3d(const Scene3DBridgeInput& input) {
    Scene3DBridgeOutput output;

#ifdef TACHYON_ENABLE_3D
    output.scene3d.camera = build_camera_3d(input.state->camera, *input.state);
    output.scene3d.lights = build_lights_3d(input.state->lights);

    if (input.plan && input.plan->scene_spec) {
        const auto comp_it = std::find_if(
            input.plan->scene_spec->compositions.begin(),
            input.plan->scene_spec->compositions.end(),
            [&](const CompositionSpec& comp) { return comp.id == input.plan->composition_target; });
        if (comp_it != input.plan->scene_spec->compositions.end() && comp_it->environment_path.has_value()) {
            output.scene3d.environment_map_id = *comp_it->environment_path;
        }
    }

    output.scene3d.instances = build_instances_3d(input);
    output.has_instances = !output.scene3d.instances.empty();
#endif

    return output;
}

} // namespace tachyon
