#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/pipeline_helpers.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/modifiers/i3d_modifier.h"
#include "tachyon/renderer3d/modifiers/three_d_modifier_registry.h"
#endif

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

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
        case scene::LayerType::Camera:
        case scene::LayerType::Light:
        case scene::LayerType::NullLayer:
            return false;
        default:
            return true;
    }
}

static LayerSurface build_layer_surface_impl(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    LayerSurface result;

    // Use pre-rendered surface from map if available
    if (input.rendered_surfaces != nullptr) {
        auto it = input.rendered_surfaces->find(l.id);
        if (it != input.rendered_surfaces->end() && it->second != nullptr) {
            result.surface = it->second;
            result.cache_key = l.id + ":" + std::to_string(input.task != nullptr ? input.task->frame_number : 0);
            result.source_kind = "surface";
            return result;
        }
    }

    // Fallback: render surface if not in map (should not happen in normal flow)
    if (input.context == nullptr || input.state == nullptr) {
        return result;
    }

    std::shared_ptr<renderer2d::SurfaceRGBA> rendered;
    if (input.plan != nullptr && input.task != nullptr) {
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
    if (rendered != nullptr) {
        result.surface = std::move(rendered);
        result.cache_key = l.id + ":" + std::to_string(input.task != nullptr ? input.task->frame_number : 0);
        result.source_kind = "surface";
    }

    return result;
}

static LayerMeshResult build_layer_mesh(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    LayerMeshResult result;

    const auto layer_surface = build_layer_surface_impl(l, input);

    // Universal surface -> plane path.
    if (layer_surface.valid()) {
        const auto textured_mesh = renderer2d::build_textured_card_mesh(
            l,
            *layer_surface.surface,
            layer_surface.cache_key.empty() ? l.id : layer_surface.cache_key);
        result.mesh_asset = textured_mesh.mesh;
        result.mesh_asset_id = textured_mesh.cache_key;
        return result;
    }

    if (l.mesh_asset) {
        result.mesh_asset = l.mesh_asset;
        result.mesh_asset_id = l.mesh_asset->path;
        if (result.mesh_asset_id.empty()) {
            result.mesh_asset_id = l.asset_path.value_or("");
        }
        return result;
    }

    // Final fallback for layers that cannot produce a surface.
    if (l.width > 0 && l.height > 0) {
        const auto solid_mesh = renderer2d::build_colored_card_mesh(l, std::to_string(input.task != nullptr ? input.task->frame_number : 0));
        result.mesh_asset = solid_mesh.mesh;
        result.mesh_asset_id = solid_mesh.cache_key;
    }

    return result;
}

static renderer3d::EvaluatedMeshInstance build_instance_for_layer(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    renderer3d::EvaluatedMeshInstance inst;
    inst.object_id = 0; // Caller sets this
    inst.material_id = 0;
    inst.world_transform = l.world_matrix;
    inst.previous_world_transform = l.previous_world_matrix;

    // Material
    inst.material.base_color = l.fill_color;
    inst.material.opacity = static_cast<float>(l.opacity);
    inst.material.metallic = l.material.metallic;
    inst.material.roughness = std::max(0.05f, l.material.roughness);
    inst.material.emission_strength = l.material.emission;
    inst.material.emission_color = l.fill_color;
    inst.material.transmission = l.material.transmission;
    inst.material.ior = l.material.ior;

    // Animation rigging
    inst.joint_matrices = l.joint_matrices;
    inst.morph_weights = l.morph_weights;

#ifdef TACHYON_ENABLE_3D
    // Build mesh for layer
    const auto mesh_result = build_layer_mesh(l, input);
    auto mesh_asset = mesh_result.mesh_asset;

    // Apply 3D modifiers
    if (l.three_d && l.three_d->enabled && mesh_asset) {
        using namespace renderer3d;
        ThreeDModifierRegistry& registry = ThreeDModifierRegistry::instance();
        
        std::shared_ptr<media::MeshAsset> modifiable_asset = std::make_shared<media::MeshAsset>(*mesh_asset);

        for (const auto& mod_spec : l.three_d->modifiers) {
            std::shared_ptr<I3DModifier> modifier = registry.create(mod_spec);
            if (modifier) {
                for (media::MeshAsset::SubMesh& submesh : modifiable_asset->sub_meshes) {
                    Mesh3D mod_mesh;
                    mod_mesh.vertices.reserve(submesh.vertices.size());
                    for (std::size_t i = 0; i < submesh.vertices.size(); ++i) {
                        const media::MeshAsset::Vertex& vtx = submesh.vertices[i];
                        mod_mesh.vertices.push_back({vtx.position, vtx.uv, vtx.normal});
                    }
                    mod_mesh.indices = submesh.indices;

                    modifier->apply(mod_mesh, l.local_time_seconds, *input.context);

                    for (std::size_t i = 0; i < submesh.vertices.size(); ++i) {
                        submesh.vertices[i].position = mod_mesh.vertices[i].position;
                        submesh.vertices[i].normal = mod_mesh.vertices[i].normal;
                    }
                }
            }
        }
        inst.mesh_asset = modifiable_asset;
        inst.mesh_asset_id = mesh_result.mesh_asset_id + "_modified";
    } else {
        inst.mesh_asset = mesh_asset;
        inst.mesh_asset_id = mesh_result.mesh_asset_id;
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

renderer3d::EvaluatedCamera3D build_camera_3d(
    const scene::EvaluatedCameraState& camera,
    const scene::EvaluatedCompositionState& /*state*/) {
    renderer3d::EvaluatedCamera3D cam;
    
#ifdef TACHYON_ENABLE_3D
    // Default camera is now always validly populated in evaluator
    cam.position = camera.position;
    cam.target = camera.point_of_interest;
    cam.up = camera.up;
    cam.fov_y = camera.fov_y_rad * 180.0f / 3.14159f; 
    cam.focal_length_mm = camera.camera.focal_length_mm;
    cam.focal_distance = camera.focus_distance;
    cam.aperture = camera.aperture;
    cam.camera_id = camera.layer_id;
    cam.is_active_camera = camera.available;

    // Extract previous position from previous_world_matrix
    math::Transform3 prev_transform = camera.previous_world_matrix.to_transform();
    cam.previous_position = prev_transform.position;
    cam.previous_target = camera.point_of_interest; // TODO: compute previous POI
    cam.previous_up = camera.up;

    // Depth of Field
    cam.blur_level = camera.blur_level;
    cam.dof_enabled = camera.dof_enabled;
#endif

    return cam;
}

std::vector<renderer3d::EvaluatedLight> build_lights_3d(
    const std::vector<scene::EvaluatedLightState>& lights) {
    std::vector<renderer3d::EvaluatedLight> lights3d;
#ifdef TACHYON_ENABLE_3D
    lights3d.reserve(lights.size());

    for (const auto& light_state : lights) {
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
        lights3d.push_back(light3d);
    }
#endif
    return lights3d;
}

std::vector<renderer3d::EvaluatedMeshInstance> build_instances_3d(
    const Scene3DBridgeInput& input) {
    std::vector<renderer3d::EvaluatedMeshInstance> instances;
    std::size_t instance_counter = 0;

    for (std::size_t block_idx : *input.block_indices) {
        if (block_idx >= input.state->layers.size()) {
            continue;
        }
        const auto& l = input.state->layers[block_idx];
        if (block_idx >= input.visible_3d_layers->size() || !(*input.visible_3d_layers)[block_idx]) {
            continue;
        }

        // Handle collapsed precomp: expand nested layers
        if (l.type == scene::LayerType::Precomp && l.collapse_transformations && l.nested_composition) {
            for (const auto& nested_layer : l.nested_composition->layers) {
                if (!is_bridge_renderable_layer(nested_layer)) {
                    continue;
                }
                // Adjust transform by precomp layer's world matrix
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
    // Build camera
    output.scene3d.camera = build_camera_3d(input.state->camera, *input.state);

    // Build lights
    output.scene3d.lights = build_lights_3d(input.state->lights);

    // Build environment map
    if (input.plan->scene_spec != nullptr) {
        const auto comp_it = std::find_if(
            input.plan->scene_spec->compositions.begin(),
            input.plan->scene_spec->compositions.end(),
            [&](const CompositionSpec& comp) { return comp.id == input.plan->composition_target; });
        if (comp_it != input.plan->scene_spec->compositions.end() && comp_it->environment_path.has_value()) {
            output.scene3d.environment_map_id = *comp_it->environment_path;
        }
    }

    // Build mesh instances (culling happens here via visible_3d_layers)
    output.scene3d.instances = build_instances_3d(input);
    output.has_instances = !output.scene3d.instances.empty();
#endif

    return output;
}

} // namespace tachyon
