#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"

namespace tachyon {

math::Vector2 apply_camera2d_transform(
    const EvaluatedCamera2D& camera,
    const LayerSpec& layer,
    float layer_parallax_factor,
    const math::Vector2& layer_position) {
    (void)layer;
    math::Vector2 parallax_offset = camera.position * (1.0f - layer_parallax_factor);
    return layer_position - parallax_offset;
}

EvaluatedCamera2D evaluate_camera2d(
    const Camera2DSpec& camera_spec,
    double time_seconds) {
    
    (void)time_seconds;
    EvaluatedCamera2D result;
    result.position = camera_spec.position.value.value_or(math::Vector2::zero());
    result.rotation = static_cast<float>(camera_spec.rotation.value.value_or(0.0) * 3.14159265 / 180.0);
    result.scale = camera_spec.scale.value.value_or(math::Vector2{1.0f, 1.0f});
    result.anchor_point = camera_spec.anchor_point.value.value_or(math::Vector2::zero());
    result.zoom = static_cast<float>(camera_spec.zoom.value.value_or(1.0));
    result.viewport_width = camera_spec.viewport_width;
    result.viewport_height = camera_spec.viewport_height;
    
    result.recalculate_matrices();
    
    return result;
}

} // namespace tachyon
