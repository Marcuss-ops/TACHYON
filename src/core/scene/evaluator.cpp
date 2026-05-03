#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/camera/camera_shake.h"
#include "tachyon/core/camera/camera_types.h"
#include "tachyon/camera_cut_contract.h"

#include <algorithm>
#include <cmath>

namespace tachyon::scene {

EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds) {

    (void)frame_number;
    EvaluatedCameraState evaluated;
    
    float comp_w = static_cast<float>(composition.width);
    float comp_h = static_cast<float>(composition.height);
    float aspect = comp_w / (comp_h > 0 ? comp_h : 1.0f);
    evaluated.aspect = aspect;

    // 1. Resolve Active Camera Layer from Cuts or first available
    std::optional<std::string> active_camera_id;
    for (const auto& cut : composition.camera_cuts) {
        if (composition_time_seconds >= cut.start_seconds && composition_time_seconds < cut.end_seconds) {
            active_camera_id = cut.camera_id;
            break;
        }
    }
    
    const EvaluatedLayerState* camera_layer = nullptr;
    if (active_camera_id) {
        for (const auto& layer : layers) {
            if (layer.id == *active_camera_id) {
                camera_layer = &layer;
                break;
            }
        }
    } else {
        for (const auto& layer : layers) {
            if (layer.type == LayerType::Camera) {
                camera_layer = &layer;
                break;
            }
        }
    }
    
    // 2. Handle Default Camera (AE-like: preserves 2D on z=0)
    if (!camera_layer) {
        evaluated.available = false;
        evaluated.name = "Default Camera";
        evaluated.camera_type = camera::CameraType::two_node;
        
        // AE Default: Positioned at center, looking at center, 
        // zoom is typically 877.77 for 1920x1080 (35mm lens)
        evaluated.zoom = (comp_h / 2.0f) / std::tan(0.683f / 2.0f); // reverse-engineered or fixed
        // Let's use a standard 35mm-equivalent for HD
        if (std::abs(comp_h - 1080.0f) < 1.0f) evaluated.zoom = 877.7778f;
        else if (std::abs(comp_h - 720.0f) < 1.0f) evaluated.zoom = 585.1852f;
        else evaluated.zoom = comp_h * 0.8127f; // heuristic

        evaluated.position = { comp_w * 0.5f, comp_h * 0.5f, -evaluated.zoom };
        evaluated.point_of_interest = { comp_w * 0.5f, comp_h * 0.5f, 0.0f };
        evaluated.up = { 0.0f, 1.0f, 0.0f }; // +Y world up convention
        
        evaluated.view_matrix = math::Matrix4x4::look_at(evaluated.position, evaluated.point_of_interest, evaluated.up);
        evaluated.fov_y_rad = 2.0f * std::atan(comp_h / (2.0f * evaluated.zoom));
        evaluated.projection_matrix = math::Matrix4x4::perspective(evaluated.fov_y_rad, evaluated.aspect, evaluated.near_clip, evaluated.far_clip);
        
        evaluated.camera.transform.position = evaluated.position;
        evaluated.camera.up = evaluated.up;
        evaluated.camera.use_target = true;
        evaluated.camera.target_position = evaluated.point_of_interest;
        evaluated.camera.fov_y_rad = evaluated.fov_y_rad;
        evaluated.camera.aspect = evaluated.aspect;
        evaluated.camera.type = camera::CameraType::two_node;

        return evaluated;
    }
    
    // 3. Evaluate Explicit Camera Layer
    evaluated.available = true;
    evaluated.layer_id = camera_layer->id;
    evaluated.name = camera_layer->name;
    evaluated.camera_type = camera_layer->camera_type;
    evaluated.zoom = camera_layer->zoom;
    evaluated.point_of_interest = camera_layer->poi;
    evaluated.position = camera_layer->world_matrix.to_transform().position;
    evaluated.up = { 0.0f, 1.0f, 0.0f }; // +Y world up convention (TODO: read from layer)

    // AE-style Optics: Zoom to FOV
    evaluated.fov_y_rad = 2.0f * std::atan(comp_h / (2.0f * evaluated.zoom));
    
    // Populate internal CameraState for view matrix computation
    evaluated.camera.transform = camera_layer->world_matrix.to_transform();
    evaluated.camera.type = evaluated.camera_type;
    evaluated.camera.use_target = (evaluated.camera_type == camera::CameraType::two_node);
    evaluated.camera.target_position = evaluated.point_of_interest;
    evaluated.camera.up = evaluated.up;
    evaluated.camera.roll = 0.0f; // TODO: read from layer properties
    evaluated.camera.fov_y_rad = evaluated.fov_y_rad;
    evaluated.camera.zoom = evaluated.zoom;
    evaluated.camera.aspect = aspect;
    evaluated.camera.near_z = evaluated.near_clip;
    evaluated.camera.far_z = evaluated.far_clip;
    
    if (evaluated.camera_type == camera::CameraType::two_node) {
        evaluated.view_matrix = evaluated.camera.get_view_matrix();
        // Previous world matrix for motion blur
        math::Vector3 prev_pos = camera_layer->previous_world_matrix.to_transform().position;
        evaluated.previous_world_matrix = math::Matrix4x4::look_at(prev_pos, evaluated.point_of_interest, evaluated.up).inverse_affine();
    } else {
        // One-node uses world matrix directly (inverted for view)
        evaluated.view_matrix = camera_layer->world_matrix.inverse_affine();
        evaluated.previous_world_matrix = camera_layer->previous_world_matrix;
    }
    
    evaluated.projection_matrix = math::Matrix4x4::perspective(evaluated.fov_y_rad, aspect, evaluated.near_clip, evaluated.far_clip);
    
    // Depth of Field (Sync with optics)
    // Read aperture from layer props (animated)
    evaluated.aperture = camera_layer->camera_aperture.value_or(2.8f);
    
    // Read focus distance from layer props, or compute from POI for two_node
    if (!camera_layer->camera_focus_distance.has_value()) {
        evaluated.focus_distance = (evaluated.camera_type == camera::CameraType::two_node) 
            ? (evaluated.point_of_interest - evaluated.position).length() 
            : evaluated.zoom;
    } else {
        evaluated.focus_distance = static_cast<float>(camera_layer->camera_focus_distance.value());
    }
    
    // Compute blur level based on aperture (relative to f/2.8 as baseline)
    // Higher aperture (lower f-number) = more blur
    evaluated.blur_level = (2.8f / std::max(evaluated.aperture, 0.5f)) * 100.0f;

    // Apply deterministic CameraShake (camera-local space)
    camera::CameraShake shake;
    shake.seed = static_cast<uint32_t>(camera_layer->camera_shake_seed);
    shake.amplitude_position = camera_layer->camera_shake_amplitude_pos;
    shake.amplitude_rotation = camera_layer->camera_shake_amplitude_rot;
    shake.frequency = camera_layer->camera_shake_frequency;
    shake.roughness = camera_layer->camera_shake_roughness;

    if (shake.is_active()) {
        float t = static_cast<float>(composition_time_seconds);
        math::Vector3 pos_off = shake.evaluate_position(t);
        math::Vector3 rot_off = shake.evaluate_rotation(t);
        
        // Apply shake in camera-local space
        // For view matrix: shake is applied as an additional transform
        math::Transform3 shake_t;
        shake_t.position = pos_off;
        shake_t.rotation = math::Quaternion::from_euler(rot_off);
        
        math::Matrix4x4 shake_mat = shake_t.to_matrix();
        // Apply shake to view: camera shake happens in camera-local space
        // This means we post-multiply for view space shake
        evaluated.view_matrix = evaluated.view_matrix * shake_mat.inverse_affine();
        
        // Also update the CameraState to keep it in sync
        evaluated.camera.transform.position += pos_off;
        evaluated.camera.transform.rotation = shake_t.rotation * evaluated.camera.transform.rotation;
    }
    
    return evaluated;
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {
    return evaluate_composition_internal(
        nullptr,
        composition,
        frame_number,
        composition_time_seconds,
        {},
        audio_analyzer,
        std::move(vars),
        media,
        std::nullopt,
        std::nullopt);
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    double fps = composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;
    double time = static_cast<double>(frame_number) / fps;
    return evaluate_composition_state(composition, frame_number, time, audio_analyzer, vars, media);
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    double fps = composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;
    std::int64_t frame = static_cast<std::int64_t>(composition_time_seconds * fps);
    return evaluate_composition_state(composition, frame, composition_time_seconds, audio_analyzer, vars, media);
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    for (const auto& comp : scene.compositions) {
        if (comp.id == composition_id) {
            double fps = comp.frame_rate.value();
            if (fps <= 0.0) fps = 24.0;
            double time = static_cast<double>(frame_number) / fps;
            return evaluate_composition_internal(
                &scene, comp, frame_number, time,
                {}, audio_analyzer, vars, media, std::nullopt, std::nullopt);
        }
    }
    return std::nullopt;
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {

    for (const auto& comp : scene.compositions) {
        if (comp.id == composition_id) {
            double fps = comp.frame_rate.value();
            if (fps <= 0.0) fps = 24.0;
            std::int64_t frame = static_cast<std::int64_t>(composition_time_seconds * fps);
            return evaluate_composition_internal(
                &scene, comp, frame, composition_time_seconds,
                {}, audio_analyzer, vars, media, std::nullopt, std::nullopt);
        }
    }
    return std::nullopt;
}

EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const audio::AudioAnalyzer* audio_analyzer,
    media::MediaManager* media) {
    CompositionSpec composition;
    composition.frame_rate = {30, 1};
    composition.layers.push_back(layer);

    EvaluationContext context{
        .scene = nullptr,
        .composition = composition,
        .frame_number = frame_number,
        .composition_time_seconds = composition_time_seconds,
        .layer_indices = {},
        .camera2d_indices = {},
        .cache = std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        .visiting = std::vector<bool>(composition.layers.size(), false),
        .composition_stack = {},
        .audio_analyzer = audio_analyzer,
        .vars = {},
        .subtitle_cache = {},
        .media = media,
        .sampler = nullptr,
        .main_frame_number = std::nullopt,
        .main_frame_time_seconds = std::nullopt
    };

    return make_layer_state(context, context.composition.layers.front(), 0, 0.0, {});
}

} // namespace tachyon::scene
