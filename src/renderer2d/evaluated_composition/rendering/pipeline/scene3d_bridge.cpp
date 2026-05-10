#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/pipeline_helpers.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/text/animation/text_animation_options.h"

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
    if (input.plan && input.task && input.intent) {
        rendered = renderer2d::render_layer_surface(
            l,
            *input.state,
            *input.intent,
            *input.plan,
            *input.task,
            const_cast<renderer2d::RenderContext2D&>(*input.context),
            std::nullopt);
    } else if (input.intent) {
        rendered = renderer2d::render_simple_layer_surface(
            l,
            *input.state,
            *input.intent,
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

    if (l.type == LayerType::Text && input.context && input.context->font_registry) {
        ::tachyon::text::TextAnimationOptions animation;
        animation.time_seconds = static_cast<float>(l.local_time_seconds);
        animation.animators = l.text_animators;

        const auto text_mesh = renderer2d::build_text_extrusion_mesh(
            l,
            *input.state,
            *input.context->font_registry,
            animation);

        if (text_mesh.mesh) {
            std::cerr << "[Scene3DBridge] text extrusion ok id=" << l.id
                      << " submeshes=" << text_mesh.mesh->sub_meshes.size()
                      << " cache=" << text_mesh.cache_key << "\n";
            if (std::getenv("TACHYON_DIAGNOSTICS") && !text_mesh.mesh->sub_meshes.empty()) {
                const auto& sub = text_mesh.mesh->sub_meshes.front();
                std::cerr << "[Scene3DBridge] text mesh origin id=" << l.id
                          << " submesh_t=(" << sub.transform[12] << ","
                          << sub.transform[13] << ","
                          << sub.transform[14] << ")"
                          << " layer_world=(" << l.world_matrix[12] << ","
                          << l.world_matrix[13] << ","
                          << l.world_matrix[14] << ")\n";
            }
            result.mesh_asset = text_mesh.mesh;
            result.mesh_asset_id = text_mesh.cache_key;
            return result;
        }

        std::cerr << "[Scene3DBridge] text extrusion fallback id=" << l.id
                  << " content_len=" << l.text_content.size()
                  << " font_id=" << l.font_id << "\n";
    }

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

    if (std::getenv("TACHYON_DIAGNOSTICS") &&
        l.type == LayerType::Text &&
        !l.id.empty()) {
        std::cerr << "[Scene3DBridge] instance transform id=" << l.id
                  << " world=(" << inst.world_transform[12] << ","
                  << inst.world_transform[13] << ","
                  << inst.world_transform[14] << ")\n";
    }

    inst.material.base_color = l.fill_color;
    inst.material.opacity = static_cast<float>(l.opacity);
    inst.material.metallic = l.material.metallic;
    inst.material.roughness = std::max(0.05f, l.material.roughness);
    inst.material.emission_strength = l.material.emission;
    inst.material.emission_color = l.fill_color;
    inst.material.transmission = l.material.transmission;
    inst.material.ior = l.material.ior;
    inst.material.subsurface = l.material.subsurface;
    inst.material.specular = l.material.specular;
    inst.material.specular_tint = l.material.specular_tint;
    inst.material.anisotropic = l.material.anisotropic;
    inst.material.sheen = l.material.sheen;
    inst.material.sheen_tint = l.material.sheen_tint;
    inst.material.clearcoat = l.material.clearcoat;
    inst.material.clearcoat_roughness = l.material.clearcoat_roughness;
    
    if (input.intent) {
        auto it = input.intent->layer_resources.find(l.id);
        if (it != input.intent->layer_resources.end()) {
            inst.layer_texture = it->second.texture_rgba;
            inst.layer_width = l.width;
            inst.layer_height = l.height;
        }
    }

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

static EvaluatedCamera3D build_camera_3d_from_layer(const scene::EvaluatedLayerState& layer) {
    EvaluatedCamera3D cam;
    cam.position = layer.world_position3;
    cam.target = layer.poi;
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fov_y = 2.0f * 180.0f / 3.14159f * std::atan(720.0f / (2.0f * std::max(layer.zoom, 1.0f)));
    cam.focal_distance = layer.camera_focus_distance.value_or(std::max(1.0f, (cam.target - cam.position).length()));
    cam.aperture = layer.camera_aperture.value_or(0.0f);
    cam.camera_id = layer.layer_id;
    cam.is_active_camera = layer.active;
    cam.previous_position = layer.previous_world_matrix.to_transform().position;
    cam.previous_target = layer.poi;
    cam.previous_up = cam.up;
    cam.blur_level = 100.0f;
    cam.dof_enabled = layer.camera_aperture.has_value();
    return cam;
}

static EvaluatedCamera3D build_camera_3d_from_spec_layer(
    const LayerSpec& layer,
    int comp_width,
    int comp_height) {
    EvaluatedCamera3D cam;
    cam.position = layer.transform3d.position_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f});
    cam.target = layer.camera_poi.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f});
    cam.up = {0.0f, 1.0f, 0.0f};
    const float zoom = static_cast<float>(layer.camera_zoom.value.value_or(877.0));
    const float safe_h = static_cast<float>(std::max(1, comp_height));
    cam.fov_y = 2.0f * 180.0f / 3.14159f * std::atan(safe_h / (2.0f * std::max(zoom, 1.0f)));
    cam.focal_distance = static_cast<float>(layer.camera_focus_distance.value.value_or((cam.target - cam.position).length()));
    cam.aperture = static_cast<float>(layer.camera_aperture.value.value_or(0.0));
    cam.camera_id = layer.id;
    cam.is_active_camera = layer.enabled && layer.visible;
    cam.previous_position = cam.position;
    cam.previous_target = cam.target;
    cam.previous_up = cam.up;
    cam.blur_level = 100.0f;
    cam.dof_enabled = layer.camera_aperture.value.has_value();
    (void)comp_width;
    return cam;
}

static EvaluatedCamera3D build_default_perspective_camera(int comp_width, int comp_height) {
    EvaluatedCamera3D cam;

    const float w = static_cast<float>(std::max(1, comp_width));
    const float h = static_cast<float>(std::max(1, comp_height));
    const float depth = std::max(w, h) * 0.95f;

    cam.position = {w * 0.44f, h * 0.36f, -depth};
    cam.target = {w * 0.52f, h * 0.50f, 0.0f};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fov_y = 2.0f * 180.0f / 3.14159f * std::atan(h / (2.0f * 877.0f));
    cam.focal_distance = std::max(1.0f, (cam.target - cam.position).length());
    cam.aperture = 0.0f;
    cam.camera_id = "default_perspective";
    cam.is_active_camera = true;
    cam.previous_position = cam.position;
    cam.previous_target = cam.target;
    cam.previous_up = cam.up;
    cam.blur_level = 100.0f;
    cam.dof_enabled = false;
    return cam;
}

static std::shared_ptr<const media::MeshAsset> build_floor_grid_mesh(int comp_width, int comp_height) {
    auto mesh = std::make_shared<media::MeshAsset>();
    media::MeshAsset::SubMesh sub;

    const float w = static_cast<float>(std::max(1, comp_width));
    const float h = static_cast<float>(std::max(1, comp_height));
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    const float span_x = w * 1.35f;
    const float span_y = h * 1.35f;
    const float thickness = std::max(8.0f, std::min(w, h) * 0.0060f);
    const float spacing = std::max(96.0f, std::min(w, h) / 10.0f);
    const math::Vector3 grid_color{0.28f, 0.29f, 0.32f};

    const auto add_quad = [&](float x0, float y0, float x1, float y1) {
        const std::uint32_t base = static_cast<std::uint32_t>(sub.vertices.size());
        sub.vertices.push_back({{x0, y0, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}});
        sub.vertices.push_back({{x1, y0, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}});
        sub.vertices.push_back({{x1, y1, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}});
        sub.vertices.push_back({{x0, y1, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
        sub.indices.insert(sub.indices.end(), {
            base + 0, base + 1, base + 2,
            base + 0, base + 2, base + 3
        });
    };

    for (float x = cx - span_x * 0.5f; x <= cx + span_x * 0.5f + 0.5f; x += spacing) {
        add_quad(x - thickness * 0.5f, cy - span_y * 0.5f, x + thickness * 0.5f, cy + span_y * 0.5f);
    }
    for (float y = cy - span_y * 0.5f; y <= cy + span_y * 0.5f + 0.5f; y += spacing) {
        add_quad(cx - span_x * 0.5f, y - thickness * 0.5f, cx + span_x * 0.5f, y + thickness * 0.5f);
    }

    sub.material.base_color_factor = grid_color;
    sub.material.metallic_factor = 0.0f;
    sub.material.roughness_factor = 1.0f;
    mesh->sub_meshes.push_back(std::move(sub));
    return mesh;
}

static std::optional<LayerSpec> find_camera_layer_spec(const SceneSpec* scene_spec, const std::string& composition_id) {
    if (!scene_spec) {
        return std::nullopt;
    }

    auto find_in_composition = [](const CompositionSpec& comp) -> std::optional<LayerSpec> {
        const auto camera_it = std::find_if(
            comp.layers.begin(),
            comp.layers.end(),
            [](const LayerSpec& layer) {
                return layer.type == LayerType::Camera && layer.enabled;
            });
        if (camera_it == comp.layers.end()) {
            return std::nullopt;
        }
        return *camera_it;
    };

    if (!composition_id.empty()) {
        const auto comp_it = std::find_if(
            scene_spec->compositions.begin(),
            scene_spec->compositions.end(),
            [&](const CompositionSpec& comp) { return comp.id == composition_id; });
        if (comp_it != scene_spec->compositions.end()) {
            if (auto camera = find_in_composition(*comp_it)) {
                return camera;
            }
        }
    }

    for (const auto& comp : scene_spec->compositions) {
        if (auto camera = find_in_composition(comp)) {
            return camera;
        }
    }

    return std::nullopt;
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

    const bool has_visible_3d_layer = std::any_of(
        input.state->layers.begin(),
        input.state->layers.end(),
        [](const auto& layer) { return layer.is_3d && layer.visible && layer.enabled && layer.active; });
    if (has_visible_3d_layer) {
        EvaluatedMeshInstance3D grid;
        grid.object_id = static_cast<std::uint32_t>(instance_counter++);
        grid.world_transform = math::Matrix4x4::identity();
        grid.previous_world_transform = grid.world_transform;
        grid.material.base_color = ColorSpec{189, 191, 199, 255};
        grid.material.opacity = 1.0f;
        grid.material.roughness = 1.0f;
        grid.material.metallic = 0.0f;
        grid.mesh_asset = build_floor_grid_mesh(input.state->width, input.state->height);
        grid.mesh_asset_id = "floor_grid";
        instances.push_back(std::move(grid));
    }

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
    if (input.state->camera.available) {
        output.scene3d.camera = build_camera_3d(input.state->camera, *input.state);
    } else {
        if (const auto camera_spec = find_camera_layer_spec(
                input.plan ? input.plan->scene_spec : nullptr,
                input.plan ? input.plan->composition_target : std::string{})) {
            output.scene3d.camera = build_camera_3d_from_spec_layer(
                *camera_spec,
                input.state->width,
                input.state->height);
        } else {
            const auto camera_it = std::find_if(
                input.state->layers.begin(),
                input.state->layers.end(),
                [](const auto& layer) { return layer.type == LayerType::Camera && layer.enabled && layer.active; });
            if (camera_it != input.state->layers.end()) {
                output.scene3d.camera = build_camera_3d_from_layer(*camera_it);
                output.scene3d.camera.is_active_camera = true;
            } else {
                output.scene3d.camera = build_camera_3d(input.state->camera, *input.state);
            }
        }
    }

    const auto camera_position_length = output.scene3d.camera.position.length();
    const auto camera_target_length = output.scene3d.camera.target.length();
    if (camera_position_length < 1e-3f && camera_target_length < 1e-3f) {
        output.scene3d.camera = build_default_perspective_camera(input.state->width, input.state->height);
    }

    std::cerr << "[Scene3DBridge] camera pos=("
              << output.scene3d.camera.position.x << ","
              << output.scene3d.camera.position.y << ","
              << output.scene3d.camera.position.z << ") target=("
              << output.scene3d.camera.target.x << ","
              << output.scene3d.camera.target.y << ","
              << output.scene3d.camera.target.z << ") type="
              << input.state->camera.camera_type
              << " available=" << (input.state->camera.available ? 1 : 0)
              << "\n";
    for (const auto& layer : input.state->layers) {
        if (layer.type == LayerType::Camera || layer.is_3d) {
            std::cerr << "[Scene3DBridge] layer id=" << layer.id
                      << " type=" << static_cast<int>(layer.type)
                      << " is_3d=" << (layer.is_3d ? 1 : 0)
                      << " enabled=" << (layer.enabled ? 1 : 0)
                      << " active=" << (layer.active ? 1 : 0)
                      << "\n";
        }
    }
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
