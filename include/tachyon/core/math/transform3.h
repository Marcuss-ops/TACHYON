#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/matrix4x4.h"

namespace tachyon {
namespace math {

/**
 * High-level 3D transform consisting of position, rotation, and scale.
 */
struct Transform3 {
    Vector3 position{Vector3::zero()};
    Quaternion rotation{Quaternion::identity()};
    Vector3 scale{Vector3::one()};

    Transform3() = default;
    Transform3(const Vector3& p, const Quaternion& r, const Vector3& s) 
        : position(p), rotation(r), scale(s) {}

    static Transform3 identity() { return {}; }

    /**
     * Composes the TRS matrix: T * R * S
     */
    [[nodiscard]] Matrix4x4 to_matrix() const;

    /**
     * Produces the inverse of the TRS matrix.
     */
    [[nodiscard]] Matrix4x4 to_inverse_matrix() const;

    void compose_trs(const Vector3& p, const Quaternion& r, const Vector3& s) {
        position = p;
        rotation = r;
        scale = s;
    }
};

} // namespace math
} // namespace tachyon
