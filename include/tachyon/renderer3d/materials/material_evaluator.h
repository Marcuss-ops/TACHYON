#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/vector3.h"

namespace tachyon::renderer3d {

struct MaterialSample {
    math::Vector3 base_color{1.0f, 1.0f, 1.0f};
    float roughness{0.5f};
    float metallic{0.0f};
    math::Vector3 normal{0.0f, 0.0f, 1.0f};
    math::Vector3 emissive{0.0f, 0.0f, 0.0f};
    
    // Disney BSDF Parameters
    float subsurface{0.0f};
    float specular{0.5f};
    float specular_tint{0.0f};
    float anisotropic{0.0f};
    float sheen{0.0f};
    float sheen_tint{0.5f};
    float clearcoat{0.0f};
    float clearcoat_roughness{0.03f};
    float ior{1.45f};
    float transmission{0.0f};

    // Consolidated contract for shaders
    math::Vector3 diffuse_color{1.0f, 1.0f, 1.0f};
    math::Vector3 specular_color{0.04f, 0.04f, 0.04f};
};

class MaterialEvaluator {
public:
    static MaterialSample evaluate(
        const scene::EvaluatedLayerState& layer,
        const math::Vector3& hit_position,
        const math::Vector3& hit_normal,
        const math::Vector2& uv);
};

} // namespace tachyon::renderer3d
