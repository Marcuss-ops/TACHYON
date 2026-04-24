#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"
#include "tachyon/core/spec/schema/objects/camera2d_spec.h"

namespace tachyon {

math::Vector2 apply_camera2d_transform(
    const EvaluatedCamera2D& camera,
    const LayerSpec& layer,
    float layer_parallax_factor,
    const math::Vector2& layer_position) {
    
    math::Vector2 parallax_offset = camera.position * (1.0f - layer_parallax_factor);
    return layer_position - parallax_offset;
}

EvaluatedCamera2D evaluate_camera2d(
    const Camera2DSpec& camera_spec,
    double time_seconds) {
    
    EvaluatedCamera2D result;
    result.position = camera_spec.position.evaluate(time_seconds);
    result.rotation = static_cast<float>(camera_spec.rotation.evaluate(time_seconds) * 3.14159265 / 180.0);
    result.scale = camera_spec.scale.evaluate(time_seconds);
    result.anchor_point = camera_spec.anchor_point.evaluate(time_seconds);
    result.zoom = static_cast<float>(camera_spec.zoom.evaluate(time_seconds));
    result.viewport_width = camera_spec.viewport_width;
    result.viewport_height = camera_spec.viewport_height;
    
    result.recalculate_matrices();
    
    return result;
}

} // namespace tachyon
