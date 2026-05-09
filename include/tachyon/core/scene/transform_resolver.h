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
    
    inline math::Vector2 apply(const math::Vector2& point) const {
        float sx = point.x * scale.x;
        float sy = point.y * scale.y;
        float rad = rotation * (3.14159265f / 180.0f);
        float cos_a = std::cos(rad);
        float sin_a = std::sin(rad);
        float rx = sx * cos_a - sy * sin_a;
        float ry = sx * sin_a + sy * cos_a;
        return {rx + translation.x, ry + translation.y};
    }
};

/**
 * @brief Resolves the final world-space transform for a layer given its bounds.
 * 
 * @param src The source transform spec.
 * @param b The size of the layer content (e.g. text box size).
 * @return ResolvedTransform2D The resolved transform.
 */
inline ResolvedTransform2D resolve_transform_2d(
    const math::Transform2& src,
    const math::Vector2& b) 
{
    (void)b;
    ResolvedTransform2D res;
    res.scale = src.scale;
    res.rotation = src.rotation_rad * (180.0f / 3.14159265f);
    
    float ax = src.anchor_point.x * res.scale.x;
    float ay = src.anchor_point.y * res.scale.y;
    float r = src.rotation_rad;
    float c = std::cos(r);
    float s = std::sin(r);
    float rx = ax * c - ay * s;
    float ry = ax * s + ay * c;
    
    res.translation.x = src.position.x - rx;
    res.translation.y = src.position.y - ry;
    return res;
}
 
} // namespace tachyon
