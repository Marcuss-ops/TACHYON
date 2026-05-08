#include "tachyon/core/scene/transform_resolver.h"
#include <cmath>

namespace tachyon {

math::Vector2 ResolvedTransform2D::apply(const math::Vector2& point) const {
    float sx = point.x * scale.x;
    float sy = point.y * scale.y;
    float rad = rotation * (3.14159265f / 180.0f);
    float cos_a = std::cos(rad);
    float sin_a = std::sin(rad);
    float rx = sx * cos_a - sy * sin_a;
    float ry = sx * sin_a + sy * cos_a;
    return {rx + translation.x, ry + translation.y};
}

ResolvedTransform2D resolve_transform_2d(const math::Transform2& src, const math::Vector2& b) {
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
