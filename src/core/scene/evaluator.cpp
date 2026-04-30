#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/camera/camera_shake.h"
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
    
    // 1. Resolve Active Camera from Cuts
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
        // Fallback to the first camera layer
        for (const auto& layer : layers) {
            if (layer.type == LayerType::Camera) {
                camera_layer = &layer;
                break;
            }
        }
    }
    
    // Default aspect ratio
    float aspect = static_cast<float>(composition.width) / (composition.height > 0 ? static_cast<float>(composition.height) : 1.0f);
    
    if (!camera_layer) {
        evaluated.available = false;
        evaluated.camera.aspect = aspect;
        evaluated.camera.transform.position = {0.0f, 0.0f, -1000.0f}; // AE default
        evaluated.camera.fov_y_rad = 0.68f; // ~39 degrees
        evaluated.previous_camera_matrix = math::Matrix4x4::identity();
        return evaluated;
    }
    
    evaluated.available = true;
    evaluated.layer_id = camera_layer->id;
    evaluated.position = camera_layer->world_matrix.to_transform().position;
    evaluated.camera_type = camera_layer->camera_type;
    evaluated.zoom = camera_layer->zoom;
    evaluated.point_of_interest = camera_layer->poi;
    evaluated.previous_position = math::Vector3{
        camera_layer->previous_world_matrix[12],
        camera_layer->previous_world_matrix[13],
        camera_layer->previous_world_matrix[14]
    };
    evaluated.previous_point_of_interest = camera_layer->poi;
    
    // AE-style Optics: Zoom to FOV
    // Formula: FOV = 2 * atan(height / (2 * zoom))
    evaluated.camera.fov_y_rad = 2.0f * std::atan(static_cast<float>(composition.height) / (2.0f * evaluated.zoom));
    evaluated.camera.aspect = aspect;

    // Resolve Camera Matrix
    if (evaluated.camera_type == "two_node") {
        // Point of Interest logic
        // Matrix4x4::look_at creates a VIEW matrix (world-to-camera).
        // AE Up vector is usually [0, 1, 0] but cameras look down -Z by default.
        math::Matrix4x4 view_matrix = math::Matrix4x4::look_at(evaluated.position, evaluated.point_of_interest, {0.0f, 1.0f, 0.0f});
        
        // CameraState::transform is camera-to-world
        evaluated.camera.transform = view_matrix.inverse_affine().to_transform();
    } else {
        // One-node camera uses the layer's world matrix
        evaluated.camera.transform = camera_layer->world_matrix.to_transform();
    }
    
    // Temporal State (Previous Camera Matrix for Motion Blur)
    evaluated.previous_camera_matrix = camera_layer->previous_world_matrix;
    
    // AE-style Optics (Secondary stats)
    evaluated.focal_length = 50.0f; // Simplified
    evaluated.film_size = 36.0f;
    evaluated.angle_of_view = evaluated.camera.fov_y_rad * 180.0f / 3.14159f;
    
    // Depth of Field
    evaluated.dof_enabled = false; 
    evaluated.focus_distance = (evaluated.camera_type == "two_node") 
        ? (evaluated.point_of_interest - evaluated.position).length() 
        : evaluated.zoom;
    evaluated.aperture = 2.8f;
    evaluated.blur_level = 100.0f;

    // Apply deterministic CameraShake
    camera::CameraShake shake;
    shake.seed = static_cast<uint32_t>(camera_layer->camera_shake_seed);
    shake.amplitude_position = camera_layer->camera_shake_amplitude_pos;
    shake.amplitude_rotation = camera_layer->camera_shake_amplitude_rot;
    shake.frequency = camera_layer->camera_shake_frequency;
    shake.roughness = camera_layer->camera_shake_roughness;

    if (shake.is_active()) {
        // Position shake
        math::Vector3 pos_off = shake.evaluate_position(static_cast<float>(composition_time_seconds));
        evaluated.camera.transform.position += pos_off;
        
        // Rotation shake (Euler degrees to Quaternion)
        math::Vector3 rot_off = shake.evaluate_rotation(static_cast<float>(composition_time_seconds));
        math::Quaternion shake_rot = math::Quaternion::from_euler(rot_off);
        evaluated.camera.transform.rotation = evaluated.camera.transform.rotation * shake_rot;
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
        nullptr,
        composition,
        frame_number,
        composition_time_seconds,
        {},
        std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        std::vector<bool>(composition.layers.size(), false),
        {},
        audio_analyzer,
        {},
        {},
        media,
        {},
        std::nullopt,
        std::nullopt
    };

    return make_layer_state(context, context.composition.layers.front(), 0, 0.0, {});
}

} // namespace tachyon::scene
