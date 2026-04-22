#include "tachyon/renderer3d/materials/material_evaluator.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer3d {

MaterialSample MaterialEvaluator::evaluate(
    const scene::EvaluatedLayerState& layer,
    const math::Vector3& hit_position,
    const math::Vector3& hit_normal,
    const math::Vector2& uv) {
    
    (void)hit_position; // Currently unused, reserved for procedural textures

    MaterialSample sample;
    
    // Base Color (from solid/image)
    sample.base_color = math::Vector3(
        layer.fill_color.r / 255.0f,
        layer.fill_color.g / 255.0f,
        layer.fill_color.b / 255.0f
    );

    // Sample the layer texture when present.
    if (layer.texture_rgba && layer.width > 0 && layer.height > 0) {
        int tx = std::clamp(static_cast<int>(uv.x * layer.width), 0, static_cast<int>(layer.width - 1));
        int ty = std::clamp(static_cast<int>(uv.y * layer.height), 0, static_cast<int>(layer.height - 1));
        std::size_t idx = (ty * layer.width + tx) * 4;
        sample.base_color.x = layer.texture_rgba[idx + 0] / 255.0f;
        sample.base_color.y = layer.texture_rgba[idx + 1] / 255.0f;
        sample.base_color.z = layer.texture_rgba[idx + 2] / 255.0f;
    }

    // Material properties
    sample.roughness = layer.material.roughness;
    sample.metallic = layer.material.metallic;
    sample.ior = layer.material.ior;

    // Disney PBR specific lobes
    sample.subsurface = layer.material.subsurface;
    sample.specular = layer.material.specular;
    sample.specular_tint = layer.material.specular_tint;
    sample.anisotropic = layer.material.anisotropic;
    sample.sheen = layer.material.sheen;
    sample.sheen_tint = layer.material.sheen_tint;
    sample.clearcoat = layer.material.clearcoat;
    sample.clearcoat_roughness = layer.material.clearcoat_roughness;
    sample.transmission = layer.material.transmission;

    // Energy Conservation & Shading Contract
    // 1. Diffuse color is black for metals
    sample.diffuse_color = sample.base_color * (1.0f - sample.metallic);
    
    // 2. Specular color: 0.04 for dielectrics (average), base_color for metals
    // F0 = lerp(0.04 * specular, base_color, metallic)
    math::Vector3 f0 = math::Vector3(0.04f, 0.04f, 0.04f) * sample.specular;
    sample.specular_color = f0 * (1.0f - sample.metallic) + sample.base_color * sample.metallic;

    sample.normal = hit_normal;

    // Emissive
    if (layer.material.emission > 0.0f) {
        sample.emissive = sample.base_color * layer.material.emission;
    } else {
        sample.emissive = math::Vector3::zero();
    }

    return sample;
}

} // namespace tachyon::renderer3d
