#pragma once

#include "tachyon/core/scene/state/evaluated_camera2d_state.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/math/matrix3x3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"

namespace tachyon {

math::Vector2 apply_camera2d_transform(
    const EvaluatedCamera2D& camera,
    const LayerSpec& layer,
    float layer_parallax_factor,
    const math::Vector2& layer_position);

EvaluatedCamera2D evaluate_camera2d(
    const Camera2DSpec& camera_spec,
    double time_seconds);

} // namespace tachyon
