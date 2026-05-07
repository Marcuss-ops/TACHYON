#include "tachyon/renderer3d/materials/material.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer3d {

std::string MaterialSystem::quality_tier_ = "high";
MaterialSystem::ProfileStats MaterialSystem::stats_;

MaterialSystem::MaterialInputs MaterialSystem::evaluate(
    const EvaluatedMeshInstance3D& instance,
    const media::MeshAsset* asset,
    const math::Vector2& uv) 
{
    (void)asset;
    (void)uv;
    MaterialInputs inputs;

    inputs.base_color = {
        instance.material.base_color.r / 255.0f,
        instance.material.base_color.g / 255.0f,
        instance.material.base_color.b / 255.0f
    };
    inputs.opacity = instance.material.opacity;
    inputs.metallic = instance.material.metallic;
    inputs.roughness = instance.material.roughness;
    inputs.emissive_intensity = instance.material.emission_strength;
    inputs.ior = instance.material.ior;
    inputs.emissive = inputs.base_color * instance.material.emission_strength;

    return inputs;
}

MaterialSystem::MaterialInputs MaterialSystem::evaluate_with_textures(
    const EvaluatedMeshInstance3D& instance,
    const media::MeshAsset* asset,
    const media::MeshAsset::SubMesh* sub_mesh,
    const math::Vector2& uv) 
{
    MaterialInputs inputs = evaluate(instance, asset, uv);

    if (!sub_mesh || !asset) {
        return inputs;
    }

    if (sub_mesh->material.base_color_texture_idx >= 0 && 
        sub_mesh->material.base_color_texture_idx < (int)asset->textures.size()) {
        const auto& tex = asset->textures[sub_mesh->material.base_color_texture_idx];
        if (!tex.data.empty() && tex.width > 0 && tex.height > 0) {
            int tx = std::clamp((int)((uv.x - std::floor(uv.x)) * tex.width), 0, tex.width - 1);
            int ty = std::clamp((int)((uv.y - std::floor(uv.y)) * tex.height), 0, tex.height - 1);
            int idx = (ty * tex.width + tx) * tex.channels;
            inputs.base_color = inputs.base_color * math::Vector3{
                tex.data[idx + 0] / 255.0f,
                tex.data[idx + 1] / 255.0f,
                tex.data[idx + 2] / 255.0f
            };
            stats_.texture_samples++;
        }
    }

    if (sub_mesh->material.metallic_roughness_texture_idx >= 0 &&
        sub_mesh->material.metallic_roughness_texture_idx < (int)asset->textures.size()) {
        const auto& tex = asset->textures[sub_mesh->material.metallic_roughness_texture_idx];
        if (!tex.data.empty() && tex.width > 0 && tex.height > 0 && tex.channels >= 3) {
            int tx = std::clamp((int)((uv.x - std::floor(uv.x)) * tex.width), 0, tex.width - 1);
            int ty = std::clamp((int)((uv.y - std::floor(uv.y)) * tex.height), 0, tex.height - 1);
            int idx = (ty * tex.width + tx) * tex.channels;
            inputs.roughness = std::max(inputs.roughness * (tex.data[idx + 1] / 255.0f), 0.05f);
            inputs.metallic = inputs.metallic * (tex.data[idx + 2] / 255.0f);
            stats_.texture_samples++;
        }
    }

    if (sub_mesh->material.normal_texture_idx >= 0 &&
        sub_mesh->material.normal_texture_idx < (int)asset->textures.size()) {
        const auto& tex = asset->textures[sub_mesh->material.normal_texture_idx];
        if (!tex.data.empty() && tex.width > 0 && tex.height > 0 && tex.channels >= 3) {
            inputs.has_normal_map = true;
            stats_.normal_map_samples++;
        }
    }

    return inputs;
}

math::Vector3 MaterialSystem::compute_normal(
    const math::Vector3& geometric_normal,
    const math::Vector3& tangent,
    const math::Vector3& bitangent,
    const math::Vector2& normal_map_value,
    float normal_strength) 
{
    math::Vector3 n = {
        normal_map_value.x * 2.0f - 1.0f,
        normal_map_value.y * 2.0f - 1.0f,
        1.0f
    };

    n.z *= normal_strength;
    n = n.normalized();

    math::Vector3 new_normal = tangent * n.x + bitangent * n.y + geometric_normal * n.z;
    return new_normal.normalized();
}

float MaterialSystem::sample_metallic_from_map(
    const std::vector<uint8_t>& data, 
    int width, 
    int height, 
    int channels, 
    int x, 
    int y) 
{
    (void)height;
    if (data.empty() || channels < 3) return 0.5f;
    int idx = (y * width + x) * channels;
    return data[idx + 2] / 255.0f;
}

math::Vector3 MaterialSystem::sample_normal_from_map(
    const std::vector<uint8_t>& data,
    int width,
    int height,
    int channels,
    int x,
    int y) 
{
    (void)height;
    if (data.empty() || channels < 3) return math::Vector3{0.0f, 0.0f, 1.0f};
    int idx = (y * width + x) * channels;
    return math::Vector3{
        (data[idx + 0] / 255.0f) * 2.0f - 1.0f,
        (data[idx + 1] / 255.0f) * 2.0f - 1.0f,
        data[idx + 2] / 255.0f
    };
}

math::Vector3 MaterialSystem::sample_emissive_from_map(
    const std::vector<uint8_t>& data,
    int width,
    int height,
    int channels,
    int x,
    int y) 
{
    (void)height;
    if (data.empty() || channels < 3) return math::Vector3::zero();
    int idx = (y * width + x) * channels;
    return math::Vector3{
        data[idx + 0] / 255.0f,
        data[idx + 1] / 255.0f,
        data[idx + 2] / 255.0f
    };
}

float MaterialSystem::sample_ao_from_map(
    const std::vector<uint8_t>& data,
    int width,
    int height,
    int channels,
    int x,
    int y) 
{
    (void)height;
    if (data.empty()) return 1.0f;
    int idx = (y * width + x) * channels;
    return data[idx] / 255.0f;
}

math::Vector2 MaterialSystem::transform_uv(
    const math::Vector2& uv,
    const TextureSlot& slot) 
{
    float u = uv.x * slot.scale_u + slot.offset_u;
    float v = uv.y * slot.scale_v + slot.offset_v;

    if (slot.wrap_mode == "clamp") {
        u = std::clamp(u, 0.0f, 1.0f);
        v = std::clamp(v, 0.0f, 1.0f);
    } else if (slot.wrap_mode == "mirror") {
        u = std::abs(u - std::floor(u));
        v = std::abs(v - std::floor(v));
    }

    return {u, v};
}

void MaterialSystem::set_quality_tier(const std::string& tier) {
    quality_tier_ = tier;
}

std::string MaterialSystem::get_quality_tier() {
    return quality_tier_;
}

const MaterialSystem::ProfileStats& MaterialSystem::get_stats() {
    return stats_;
}

void MaterialSystem::reset_stats() {
    stats_ = {};
}

} // namespace tachyon::renderer3d
