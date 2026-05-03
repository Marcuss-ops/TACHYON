#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/pipeline_helpers.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include "tachyon/text/fonts/core/font.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <optional>

namespace tachyon {

namespace {

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

struct LayerMeshResult {
    std::shared_ptr<const media::MeshAsset> mesh_asset;
    std::string mesh_asset_id;
};

static LayerMeshResult build_layer_mesh(
    const scene::EvaluatedLayerState& l,
    const Scene3DBridgeInput& input) {
    LayerMeshResult result;

    // Text mesh
    if (l.type == scene::LayerType::Text && input.context->font_registry != nullptr) {
        ::tachyon::text::TextAnimationOptions animation{};
        animation.time_seconds = static_cast<float>(l.local_time_seconds);
        animation.animators = std::span<const ::tachyon::TextAnimatorSpec>(
            l.text_animators.data(), l.text_animators.size());
        const auto text_mesh = renderer2d::build_text_extrusion_mesh(l, *input.state, *input.context->font_registry, animation);
        if (text_mesh.mesh) {
            result.mesh_asset = text_mesh.mesh;
            result.mesh_asset_id = text_mesh.cache_key;
            return result;
        }
    }

    // Media mesh
    if (l.type == scene::LayerType::Image || l.type == scene::LayerType::Video) {
        const auto media_source = resolve_media_source(l, *input.context);
        if (media_source.has_value()) {
            const ::tachyon::renderer2d::SurfaceRGBA* media_frame = nullptr;
            if (l.type == scene::LayerType::Video) {
                media_frame = input.context->media_manager->get_video_frame(*media_source, input.task->time_seconds);
            } else {
                media_frame = input.context->media_manager->get_image(*media_source);
            }

            if (media_frame != nullptr) {
                const auto media_mesh = renderer2d::build_textured_card_mesh(
                    l,
                    *media_frame,
                    media_source->generic_string() + "@" + std::to_string(input.task->time_seconds));
                result.mesh_asset = media_mesh.mesh;
                result.mesh_asset_id = media_mesh.cache_key;
                return result;
            } else {
                const auto fallback_mesh = renderer2d::build_colored_card_mesh(
                    l,
                    media_source->generic_string());
                result.mesh_asset = fallback_mesh.mesh;
                result.mesh_asset_id = fallback_mesh.cache_key;
                return result;
            }
        }
    }

    // Solid mesh
    if (l.width > 0 && l.height > 0) {
        const auto solid_mesh = renderer2d::build_colored_card_mesh(l, std::to_string(input.task->frame_number));
        result.mesh_asset = solid_mesh.mesh;
        result.mesh_asset_id = solid_mesh.cache_key;
        return result;
    }

    return result;
}

} // anonymous namespace

renderer3d::EvaluatedCamera3D build_camera_3d(
    const scene::EvaluatedCameraState& camera,
    const scene::EvaluatedCompositionState& /*state*/) {
    renderer3d::EvaluatedCamera3D cam;
    
    // Default camera is now always validly populated in evaluator
    cam.position = camera.position;
    cam.target = camera.point_of_interest;
    cam.up = camera.up;
    cam.fov_y = camera.fov_y_rad * 180.0f / 3.14159f; 
    cam.focal_length_mm = 50.0f; 
    cam.focal_distance = camera.focus_distance;
    cam.aperture = camera.aperture;
    cam.camera_id = camera.layer_id;
    cam.is_active_camera = true;

    cam.previous_position = camera.position;
    cam.previous_target = camera.point_of_interest;
    cam.previous_up = camera.up;

    return cam;
}

std::vector<renderer3d::EvaluatedLight> build_lights_3d(
    const std::vector<scene::EvaluatedLightState>& lights) {
    std::vector<renderer3d::EvaluatedLight> lights3d;
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

    return lights3d;
}

std::vector<renderer3d::EvaluatedMeshInstance> build_instances_3d(
    const Scene3DBridgeInput& input) {
    std::vector<renderer3d::EvaluatedMeshInstance> instances;
    instances.reserve(input.block_indices->size());

    for (std::size_t block_idx : *input.block_indices) {
        if (block_idx >= input.state->layers.size()) {
            continue;
        }
        const auto& l = input.state->layers[block_idx];
        if (block_idx >= input.visible_3d_layers->size() || !(*input.visible_3d_layers)[block_idx]) {
            continue;
        }
        if (!is_bridge_renderable_layer(l)) {
            continue;
        }

        renderer3d::EvaluatedMeshInstance inst;
        inst.object_id = static_cast<std::uint32_t>(block_idx);
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

        // Build mesh for layer
        const auto mesh_result = build_layer_mesh(l, input);
        inst.mesh_asset = mesh_result.mesh_asset;
        inst.mesh_asset_id = mesh_result.mesh_asset_id;

        // Final fallback to layer's asset path
        if (inst.mesh_asset_id.empty()) {
            inst.mesh_asset_id = l.asset_path.value_or("");
        }

        instances.push_back(inst);
    }

    return instances;
}

Scene3DBridgeOutput build_evaluated_scene_3d(const Scene3DBridgeInput& input) {
    Scene3DBridgeOutput output;

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

    return output;
}

} // namespace tachyon
