#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include <cmath>

namespace tachyon::scene {

EvaluatedCameraState evaluate_camera2d_state(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    double time_seconds,
    const audio::IAudioAnalyzer* audio_analyzer,
    const EvaluationVariables& vars) {
    
    EvaluatedCameraState cam_state;
    
    if (composition.cameras_2d.empty()) {
        return cam_state;
    }

    const Camera2DSpec* active_cam = nullptr;
    if (composition.active_camera2d_id.has_value() && !composition.active_camera2d_id->empty()) {
        for (const auto& cam : composition.cameras_2d) {
            if (cam.id == *composition.active_camera2d_id) {
                active_cam = &cam;
                break;
            }
        }
    }
    
    if (!active_cam) active_cam = &composition.cameras_2d[0];

    if (active_cam && active_cam->enabled) {
        cam_state.id = active_cam->id;
        cam_state.active = true;
        
        const double t = time_seconds;
        const std::uint64_t cam_seed = stable_string_hash(active_cam->id);
        
        // Resolve audio bands once
        ::tachyon::audio::AudioBands bands = audio_analyzer ? audio_analyzer->analyze_frame(t) : ::tachyon::audio::AudioBands{};

        const float zoom = static_cast<float>(sample_scalar(active_cam->zoom, 1.0, t, bands, hash_combine(cam_seed, stable_string_hash("zoom"))));
        const math::Vector2 pos = sample_vector2(active_cam->position, {static_cast<float>(composition.width)/2.0f, static_cast<float>(composition.height)/2.0f}, t, bands, hash_combine(cam_seed, stable_string_hash("pos")));
        const float rot = static_cast<float>(sample_scalar(active_cam->rotation, 0.0, t, bands, hash_combine(cam_seed, stable_string_hash("rot"))));
        const math::Vector2 scale = sample_vector2(active_cam->scale, {100.0f, 100.0f}, t, bands, hash_combine(cam_seed, stable_string_hash("scale")));
        const math::Vector2 anchor = sample_vector2(active_cam->anchor_point, {0.0f, 0.0f}, t, bands, hash_combine(cam_seed, stable_string_hash("anchor")));

        cam_state.zoom = zoom;

        // View matrix is the inverse of the camera's world transform
        math::Transform2 cam_xform;
        cam_xform.position = pos;
        cam_xform.rotation_rad = static_cast<float>(degrees_to_radians(rot));
        cam_xform.scale = {scale.x / 100.0f, scale.y / 100.0f};
        cam_xform.anchor_point = anchor;

        cam_state.view_matrix = cam_xform.to_matrix().inverse();
        
        // Projection: zoom around the center of the composition
        const float cx = static_cast<float>(composition.width) * 0.5f;
        const float cy = static_cast<float>(composition.height) * 0.5f;
        
        cam_state.view_projection = 
            math::Matrix3x3::make_translation({cx, cy}) *
            math::Matrix3x3::make_scale(zoom, zoom) *
            math::Matrix3x3::make_translation({-cx, -cy});

        cam_state.view_projection = cam_state.view_projection * cam_state.view_matrix;
    }
    
    (void)scene; // Unused
    (void)vars;  // Unused
    
    return cam_state;
}

} // namespace tachyon::scene
