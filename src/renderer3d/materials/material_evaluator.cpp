#include "tachyon/renderer3d/materials/material_evaluator.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer3d {

MaterialSample MaterialEvaluator::evaluate(
    const EvaluatedMeshInstance3D& instance,
    const math::Vector3& hit_position,
    const math::Vector3& hit_normal,
    const math::Vector2& uv) {
    
    (void)hit_position; // Currently unused, reserved for procedural textures

    MaterialSample sample;
    
    // Base Color
    sample.base_color = math::Vector3(
        instance.material.base_color.r / 255.0f,
        instance.material.base_color.g / 255.0f,
        instance.material.base_color.b / 255.0f
    );

    // Sample the layer texture when present (e.g. video, image)
    if (instance.layer_texture && instance.layer_width > 0 && instance.layer_height > 0) {
        int tx = std::clamp(static_cast<int>(uv.x * instance.layer_width), 0, instance.layer_width - 1);
        int ty = std::clamp(static_cast<int>(uv.y * instance.layer_height), 0, instance.layer_height - 1);
        std::size_t idx = (ty * instance.layer_width + tx) * 4;
        sample.base_color.x = instance.layer_texture[idx + 0] / 255.0f;
        sample.base_color.y = instance.layer_texture[idx + 1] / 255.0f;
        sample.base_color.z = instance.layer_texture[idx + 2] / 255.0f;
    }

    // Material properties
    sample.roughness = instance.material.roughness;
    sample.metallic = instance.material.metallic;
    sample.ior = instance.material.ior;

    // Disney PBR specific lobes
    sample.subsurface = instance.material.subsurface;
    sample.specular = instance.material.specular;
    sample.specular_tint = instance.material.specular_tint;
    sample.anisotropic = instance.material.anisotropic;
    sample.sheen = instance.material.sheen;
    sample.sheen_tint = instance.material.sheen_tint;
    sample.clearcoat = instance.material.clearcoat;
    sample.clearcoat_roughness = instance.material.clearcoat_roughness;
    sample.transmission = instance.material.transmission;

    // Energy Conservation & Shading Contract
    // 1. Diffuse color is black for metals
    sample.diffuse_color = sample.base_color * (1.0f - sample.metallic);
    
    // 2. Specular color: 0.04 for dielectrics (average), base_color for metals
    // F0 = lerp(0.04 * specular, base_color, metallic)
    math::Vector3 f0 = math::Vector3(0.04f, 0.04f, 0.04f) * sample.specular;
    sample.specular_color = f0 * (1.0f - sample.metallic) + sample.base_color * sample.metallic;

    sample.normal = hit_normal;

    // Emissive
    if (instance.material.emission_strength > 0.0f) {
        sample.emissive = sample.base_color * instance.material.emission_strength;
    } else {
        sample.emissive = math::Vector3::zero();
    }

    return sample;
}

} // namespace tachyon::renderer3d
