#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/math/matrix4x4.h"
#include <cmath>

namespace tachyon {
namespace renderer3d {

Tilt3DModifier::Tilt3DModifier() {}

void Tilt3DModifier::apply(
    Mesh3D& mesh,
    const ResolvedModifier3D& resolved,
    const renderer2d::RenderContext& /*ctx*/
) {
    auto get_scalar = [&](const std::string& name, float fallback) {
        auto it = resolved.scalar_params.find(name);
        if (it != resolved.scalar_params.end()) {
            return it->second;
        }
        return fallback;
    };

    float tilt_x = get_scalar("tilt_x", 0.0f);
    float tilt_y = get_scalar("tilt_y", 0.0f);

    constexpr float kDegToRad = static_cast<float>(M_PI / 180.0);
    float rad_x = tilt_x * kDegToRad;
    float rad_y = tilt_y * kDegToRad;

    math::Matrix4x4 rotation = math::Matrix4x4::rotation_x(rad_x) * math::Matrix4x4::rotation_y(rad_y);

    for (auto& vertex : mesh.vertices) {
        vertex.position = rotation.transform_point(vertex.position);
        vertex.normal = rotation.transform_vector(vertex.normal);
    }
}

} // namespace renderer3d
} // namespace tachyon
