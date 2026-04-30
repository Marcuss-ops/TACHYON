#include "tachyon/core/scene/evaluator/layer_transform_3d.h"

namespace tachyon::scene {

LayerTransform3DResult build_layer_transform_3d(const LayerTransform3DInput& input) {
    constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

    const math::Vector3 total_rotation_deg = input.orientation_deg + input.rotation_deg;
    const math::Matrix4x4 translation = math::Matrix4x4::translation(input.position);
    const math::Matrix4x4 rotation_x = math::Matrix4x4::rotation_x(total_rotation_deg.x * kDegToRad);
    const math::Matrix4x4 rotation_y = math::Matrix4x4::rotation_y(total_rotation_deg.y * kDegToRad);
    const math::Matrix4x4 rotation_z = math::Matrix4x4::rotation_z(total_rotation_deg.z * kDegToRad);
    const math::Matrix4x4 rotation = rotation_x * rotation_y * rotation_z;
    const math::Matrix4x4 anchor = math::Matrix4x4::translation({
        -input.anchor_point.x,
        -input.anchor_point.y,
        -input.anchor_point.z
    });
    const math::Matrix4x4 scale_mat = math::Matrix4x4::scaling(input.scale);

    LayerTransform3DResult result;
    result.world_matrix = translation * (rotation * (scale_mat * anchor));
    result.world_position3 = result.world_matrix.transform_point({0.0f, 0.0f, 0.0f});
    result.world_normal = result.world_matrix.transform_vector({0.0f, 0.0f, 1.0f}).normalized();

    return result;
}

} // namespace tachyon::scene
