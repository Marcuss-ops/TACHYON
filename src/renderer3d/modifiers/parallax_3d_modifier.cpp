#include "tachyon/renderer3d/modifiers/parallax_3d_modifier.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/math/matrix4x4.h"

namespace tachyon {
namespace renderer3d {

Parallax3DModifier::Parallax3DModifier(const ThreeDModifierSpec& spec)
    : spec_(spec) {}

void Parallax3DModifier::apply(
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

    float depth = static_cast<float>(get_scalar("depth", 0.0));
    // float camera_influence = static_cast<float>(get_scalar("camera_influence", 1.0));

    // Simple 3D parallax: shift vertices in Z
    // This works automatically with a perspective camera.
    math::Matrix4x4 translation = math::Matrix4x4::translation({0.0f, 0.0f, depth});

    for (auto& vertex : mesh.vertices) {
        vertex.position = translation.transform_point(vertex.position);
    }
}

} // namespace renderer3d
} // namespace tachyon
