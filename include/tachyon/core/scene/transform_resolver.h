#pragma once

#include "tachyon/core/spec/schema/transform/transform_spec.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/transform2.h"

namespace tachyon {

/**
 * @brief Resolves a 2D transform including anchor point logic.
 * 
 * The resolved world position is calculated as:
 * world_pos = position - (anchor_point * scale)
 * (Actually, rotation also affects this, so it's better to think in terms of matrix transformations).
 */
struct ResolvedTransform2D {
    math::Vector2 translation;
    math::Vector2 scale{1.0f, 1.0f};
    float rotation{0.0f};
    
    math::Vector2 apply(const math::Vector2& point) const;
};

/**
 * @brief Resolves the final world-space transform for a layer given its bounds.
 * 
 * @param transform The source transform spec.
 * @param bounds The size of the layer content (e.g. text box size).
 * @return ResolvedTransform2D The resolved transform.
 */
ResolvedTransform2D resolve_transform_2d(
    const math::Transform2& transform,
    const math::Vector2& bounds);

} // namespace tachyon
