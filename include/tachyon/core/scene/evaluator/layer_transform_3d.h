#pragma once

#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/math/algebra/matrix4x4.h"

namespace tachyon::scene {

struct LayerTransform3DInput {
    math::Vector3 position;
    math::Vector3 orientation_deg;
    math::Vector3 rotation_deg;
    math::Vector3 scale;
    math::Vector3 anchor_point;
};

struct LayerTransform3DResult {
    math::Matrix4x4 world_matrix;
    math::Vector3 world_position3;
    math::Vector3 world_normal;
};

/**
 * @brief Builds a 3D world transform from layer properties.
 *
 * This is the single shared implementation used by both:
 * - scene::make_layer_state (evaluator)
 * - frame_executor::evaluate_layer (runtime)
 *
 * The function combines:
 * - Translation (position)
 * - Rotation (orientation_deg + rotation_deg, applied in XYZ order)
 * - Anchor point offset (applied before scale)
 * - Scale
 */
LayerTransform3DResult build_layer_transform_3d(const LayerTransform3DInput& input);

} // namespace tachyon::scene
