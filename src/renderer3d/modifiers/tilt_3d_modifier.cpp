#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/renderer2d/math/math_utils.h"

namespace tachyon {
namespace renderer3d {

Tilt3DModifier::Tilt3DModifier(const ThreeDModifierSpec& spec)
    : spec_(spec) {}

void Tilt3DModifier::apply(
    Mesh3D& mesh,
    double time,
    const renderer2d::RenderContext& /*ctx*/
) {
    auto get_scalar = [&](const std::string& name, double fallback) {
        auto it = spec_.scalar_params.find(name);
        if (it != spec_.scalar_params.end()) {
            return scene::sample_scalar(it->second, fallback, time);
        }
        return fallback;
    };

    float tilt_x = static_cast<float>(get_scalar("tilt_x", 0.0));
    float tilt_y = static_cast<float>(get_scalar("tilt_y", 0.0));

    float rad_x = static_cast<float>(renderer2d::math_utils::degrees_to_radians(tilt_x));
    float rad_y = static_cast<float>(renderer2d::math_utils::degrees_to_radians(tilt_y));

    math::Matrix4x4 rotation = math::Matrix4x4::rotation_x(rad_x) * math::Matrix4x4::rotation_y(rad_y);

    for (auto& vertex : mesh.vertices) {
        vertex.position = rotation.transform_point(vertex.position);
        vertex.normal = rotation.transform_vector(vertex.normal);
    }
}

} // namespace renderer3d
} // namespace tachyon
