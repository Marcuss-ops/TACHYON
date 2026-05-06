#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/materials/pbr_utils.h"
#include "tachyon/core/math/matrix4x4.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <random>
#include <iostream>

namespace tachyon::renderer3d {

using namespace pbr;

namespace {

constexpr float kTextureEpsilon = 1e-6f;

math::Vector3 transform_normal(const math::Matrix4x4& matrix, const math::Vector3& normal) {
    const math::Matrix4x4 inv = matrix.inverse_affine();
    math::Vector3 transformed {
        inv[0] * normal.x + inv[4] * normal.y + inv[8] * normal.z,
        inv[1] * normal.x + inv[5] * normal.y + inv[9] * normal.z,
        inv[2] * normal.x + inv[6] * normal.y + inv[10] * normal.z
    };
    return transformed.normalized();
}

math::Vector3 sample_texture_bilinear(const media::TextureData& tex, const math::Vector2& uv) {
    if (tex.data.empty() || tex.width <= 0 || tex.height <= 0 || tex.channels <= 0) {
        return math::Vector3{1.0f, 1.0f, 1.0f};
    }

    const float u = uv.x - std::floor(uv.x);
    const float v = uv.y - std::floor(uv.y);
    const float x = u * static_cast<float>(std::max(1, tex.width - 1));
    const float y = (1.0f - v) * static_cast<float>(std::max(1, tex.height - 1));

    const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, tex.width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, tex.height - 1);
    const int x1 = std::clamp(x0 + 1, 0, tex.width - 1);
    const int y1 = std::clamp(y0 + 1, 0, tex.height - 1);

    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    auto texel = [&](int px, int py) -> math::Vector3 {
        const std::size_t idx = (static_cast<std::size_t>(py) * static_cast<std::size_t>(tex.width) + static_cast<std::size_t>(px)) * static_cast<std::size_t>(tex.channels);
        const float r = tex.data[idx + 0] / 255.0f;
        const float g = tex.channels > 1 ? tex.data[idx + 1] / 255.0f : r;
        const float b = tex.channels > 2 ? tex.data[idx + 2] / 255.0f : r;
        return {r, g, b};
    };

    const math::Vector3 c00 = texel(x0, y0);
    const math::Vector3 c10 = texel(x1, y0);
    const math::Vector3 c01 = texel(x0, y1);
    const math::Vector3 c11 = texel(x1, y1);
    const math::Vector3 cx0 = c00 * (1.0f - tx) + c10 * tx;
    const math::Vector3 cx1 = c01 * (1.0f - tx) + c11 * tx;
    return cx0 * (1.0f - ty) + cx1 * ty;
}

math::Vector3 safe_normalize(const math::Vector3& v, const math::Vector3& fallback) {
    const float len2 = v.length_squared();
    if (len2 > kTextureEpsilon) {
        return v / std::sqrt(len2);
    }
    return fallback;
}

bool is_finite_vector3(const math::Vector3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

} // namespace

ShadingResult RayTracer::trace_ray(
    const math::Vector3& origin,
    const math::Vector3& direction,
    const EvaluatedScene3D& scene,
    std::mt19937& rng,
    int depth,
    float time) {

    if (depth > kMaxBounces) {
        ShadingResult result;
        result.color = math::Vector3{0.0f, 0.0f, 0.0f};
        result.alpha = 0.0f;
        result.depth = 1e6f;
        result.normal = math::Vector3{0.0f, 0.0f, 1.0f};
        result.albedo = math::Vector3{0.0f, 0.0f, 0.0f};
        result.motion_vector = math::Vector2{0.0f, 0.0f};
        result.object_id = 0;
        result.material_id = 0;
        return result;
    }

    static std::atomic<int> debug_trace_count{0};
    const int trace_id = debug_trace_count.fetch_add(1);
    if (trace_id < 20) {
        std::cerr << "[RayTracer] trace #" << trace_id
                  << " depth=" << depth
                  << " origin=(" << origin.x << "," << origin.y << "," << origin.z << ")"
                  << " dir=(" << direction.x << "," << direction.y << "," << direction.z << ")"
                  << " finite=" << (is_finite_vector3(origin) && is_finite_vector3(direction) ? "yes" : "no")
                  << "\n";
    }

    // Embree ray hit structure
    RTCRayHit rh{};
    if (!is_finite_vector3(origin) || !is_finite_vector3(direction) || direction.length_squared() <= kTextureEpsilon) {
        ShadingResult result;
        result.color = math::Vector3{0.0f, 0.0f, 0.0f};
        result.alpha = 0.0f;
        result.depth = 1e6f;
        result.normal = math::Vector3{0.0f, 0.0f, 1.0f};
        result.albedo = math::Vector3{0.0f, 0.0f, 0.0f};
        result.motion_vector = math::Vector2{0.0f, 0.0f};
        result.object_id = 0;
        result.material_id = 0;
        return result;
    }

    rh.ray.org_x = origin.x; rh.ray.org_y = origin.y; rh.ray.org_z = origin.z;
    rh.ray.dir_x = direction.x; rh.ray.dir_y = direction.y; rh.ray.dir_z = direction.z;
    rh.ray.tnear = 0.001f;
    rh.ray.tfar = 100000.0f;
    rh.ray.mask = static_cast<unsigned int>(-1);
    rh.ray.time = time;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    rtcIntersect1(scene_, &rh, &args);

    if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        // Miss - sample environment
        ShadingResult res;
        res.color = {0.05f, 0.05f, 0.1f}; // Dark blue environment fallback
        res.alpha = 0.0f;
        res.depth = 1000000.0f;
        return res;
    }
    // Hit - find instance
    const GeoInstance* hit_instance = nullptr;
    for (const auto& inst : instances_) {
        if (inst.geom_id == rh.hit.geomID || inst.geom_id == rh.hit.instID[0]) {
            hit_instance = &inst;
            break;
        }
    }

    if (!hit_instance) {
        // Magenta error color
        ShadingResult result;
        result.color = math::Vector3{1.0f, 0.0f, 1.0f};
        result.alpha = 1.0f;
        result.depth = rh.ray.tfar;
        result.normal = math::Vector3{1.0f, 0.0f, 1.0f};
        result.albedo = math::Vector3{0.0f, 0.0f, 0.0f};
        result.motion_vector = math::Vector2{0.0f, 0.0f};
        result.object_id = 0;
        result.material_id = 0;
        return result;
    }

    const math::Vector3 hit_pos = origin + direction * rh.ray.tfar;

    const MeshCacheEntry* mesh_entry = nullptr;
    auto mesh_it = mesh_cache_.find(hit_instance->mesh_asset_id);
    if (mesh_it != mesh_cache_.end()) {
        mesh_entry = &mesh_it->second;
    }

    const MeshCacheEntry::SubMeshCache* submesh = nullptr;
    if (mesh_entry) {
        auto geom_it = mesh_entry->geom_id_to_submesh.find(rh.hit.geomID);
        if (geom_it == mesh_entry->geom_id_to_submesh.end() && rh.hit.instID[0] != RTC_INVALID_GEOMETRY_ID) {
            geom_it = mesh_entry->geom_id_to_submesh.find(rh.hit.instID[0]);
        }
        if (geom_it != mesh_entry->geom_id_to_submesh.end() && geom_it->second < mesh_entry->submeshes.size()) {
            submesh = &mesh_entry->submeshes[geom_it->second];
        }
        if (!submesh && !mesh_entry->submeshes.empty()) {
            submesh = &mesh_entry->submeshes.front();
        }
    }

    math::Vector3 geometric_normal = {rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z};
    geometric_normal = safe_normalize(geometric_normal, math::Vector3{0.0f, 0.0f, 1.0f});

    math::Vector3 world_normal = geometric_normal;
    math::Vector2 hit_uv{0.0f, 0.0f};
    math::Vector3 tangent_world{0.0f, 0.0f, 0.0f};
    math::Vector3 bitangent_world{0.0f, 0.0f, 0.0f};
    math::Vector3 interpolated_normal = geometric_normal;

    if (submesh && submesh->vertices.size() >= 3 && submesh->indices.size() >= 3) {
        const std::size_t tri_index = static_cast<std::size_t>(rh.hit.primID) * 3U;
        if (tri_index + 2U < submesh->indices.size()) {
            const unsigned int i0 = submesh->indices[tri_index + 0U];
            const unsigned int i1 = submesh->indices[tri_index + 1U];
            const unsigned int i2 = submesh->indices[tri_index + 2U];
            if (i0 < submesh->vertices.size() && i1 < submesh->vertices.size() && i2 < submesh->vertices.size()) {
                const auto& v0 = submesh->vertices[i0];
                const auto& v1 = submesh->vertices[i1];
                const auto& v2 = submesh->vertices[i2];
                const float w0 = std::max(0.0f, 1.0f - rh.hit.u - rh.hit.v);
                const float w1 = rh.hit.u;
                const float w2 = rh.hit.v;

                interpolated_normal = safe_normalize(
                    v0.normal * w0 + v1.normal * w1 + v2.normal * w2,
                    geometric_normal);
                hit_uv = v0.uv * w0 + v1.uv * w1 + v2.uv * w2;

                const math::Vector3 p0 = v0.position;
                const math::Vector3 p1 = v1.position;
                const math::Vector3 p2 = v2.position;
                const math::Vector3 edge1 = p1 - p0;
                const math::Vector3 edge2 = p2 - p0;
                const math::Vector2 duv1 = v1.uv - v0.uv;
                const math::Vector2 duv2 = v2.uv - v0.uv;
                const float denom = duv1.x * duv2.y - duv1.y * duv2.x;
                if (std::abs(denom) > kTextureEpsilon) {
                    const float inv = 1.0f / denom;
                    const math::Vector3 tangent = (edge1 * duv2.y - edge2 * duv1.y) * inv;
                    const math::Vector3 bitangent = (edge2 * duv1.x - edge1 * duv2.x) * inv;
                    tangent_world = safe_normalize(hit_instance->world_transform.transform_vector(tangent), math::Vector3{1.0f, 0.0f, 0.0f});
                    bitangent_world = safe_normalize(hit_instance->world_transform.transform_vector(bitangent), math::Vector3{0.0f, 1.0f, 0.0f});
                }
            }
        }
    }

    world_normal = safe_normalize(
        transform_normal(hit_instance->world_transform, interpolated_normal),
        geometric_normal);

    const EvaluatedMaterial3D& mat = hit_instance->material;
    math::Vector3 albedo = {
        mat.base_color.r / 255.0f,
        mat.base_color.g / 255.0f,
        mat.base_color.b / 255.0f
    };
    float metallic = std::clamp(mat.metallic, 0.0f, 1.0f);
    float roughness = std::max(mat.roughness, 0.05f);

    if (mesh_entry && mesh_entry->asset && submesh) {
        const auto& material = submesh->material;
        if (material.base_color_texture_idx >= 0 && material.base_color_texture_idx < static_cast<int>(mesh_entry->asset->textures.size())) {
            const auto& tex = mesh_entry->asset->textures[material.base_color_texture_idx];
            const math::Vector3 tex_color = sample_texture_bilinear(tex, hit_uv);
            albedo = albedo * tex_color;
        }
        if (material.metallic_roughness_texture_idx >= 0 && material.metallic_roughness_texture_idx < static_cast<int>(mesh_entry->asset->textures.size())) {
            const auto& tex = mesh_entry->asset->textures[material.metallic_roughness_texture_idx];
            const math::Vector3 tex_mr = sample_texture_bilinear(tex, hit_uv);
            roughness = std::max(0.05f, roughness * tex_mr.y);
            metallic = std::clamp(metallic * tex_mr.z, 0.0f, 1.0f);
        }
        if (material.normal_texture_idx >= 0 && material.normal_texture_idx < static_cast<int>(mesh_entry->asset->textures.size())) {
            const auto& tex = mesh_entry->asset->textures[material.normal_texture_idx];
            const math::Vector3 normal_sample = sample_texture_bilinear(tex, hit_uv) * 2.0f - math::Vector3{1.0f, 1.0f, 1.0f};
            math::Vector3 tangent_normal = safe_normalize(normal_sample, math::Vector3{0.0f, 0.0f, 1.0f});
            if (tangent_world.length_squared() > kTextureEpsilon && bitangent_world.length_squared() > kTextureEpsilon) {
                world_normal = safe_normalize(
                    tangent_world * tangent_normal.x +
                    bitangent_world * tangent_normal.y +
                    world_normal * tangent_normal.z,
                    world_normal);
            }
        }
    }

    math::Vector3 view_dir = (-direction).normalized();
    float n_dot_v = std::clamp(math::Vector3::dot(world_normal, view_dir), 0.0f, 1.0f);

    math::Vector3 total_radiance = {0.0f, 0.0f, 0.0f};

    // Direct lighting
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (const auto& light : scene.lights) {
        math::Vector3 light_dir;
        float distance = INFINITY;
        float attenuation = 1.0f;

        if (light.type == LightType3D::Ambient) {
            total_radiance += albedo * light.intensity * 0.1f;
            continue;
        } else if (light.type == LightType3D::Directional) {
            light_dir = (-light.direction).normalized();
        } else {
            math::Vector3 light_pos = light.position;
            math::Vector3 delta = light_pos - hit_pos;
            distance = delta.length();
            if (!std::isfinite(distance) || distance <= kTextureEpsilon) {
                continue;
            }
            light_dir = delta / distance;

            // Attenuation (simple inverse square for now)
            if (distance > 0.0f) {
                attenuation = 1.0f / (distance * distance);
            }

            // Spotlight
            if (light.type == LightType3D::Spot) {
                float cos_angle = math::Vector3::dot(light_dir, (-light.direction).normalized());
                float cos_outer = std::cos(light.spot_angle * 0.5f * PI / 180.0f);
                float cos_inner = std::cos(std::max(0.0f, light.spot_angle * 0.5f - light.spot_penumbra) * PI / 180.0f);
                float spot_factor = std::clamp((cos_angle - cos_outer) / (cos_inner - cos_outer), 0.0f, 1.0f);
                attenuation *= spot_factor;
            }
        }

        if (attenuation <= 0.0f) continue;

        // Shadow ray
        bool in_shadow = false;
            if (light.casts_shadows) {
                RTCRay shadow_ray{};
                shadow_ray.org_x = hit_pos.x + world_normal.x * 0.001f;
                shadow_ray.org_y = hit_pos.y + world_normal.y * 0.001f;
                shadow_ray.org_z = hit_pos.z + world_normal.z * 0.001f;
                shadow_ray.dir_x = light_dir.x;
                shadow_ray.dir_y = light_dir.y;
                shadow_ray.dir_z = light_dir.z;
                shadow_ray.tnear = 0.0f;
                shadow_ray.tfar = distance - 0.002f;
                shadow_ray.mask = static_cast<unsigned int>(-1);
                shadow_ray.time = time;

                if (!is_finite_vector3({shadow_ray.org_x, shadow_ray.org_y, shadow_ray.org_z}) ||
                    !is_finite_vector3({shadow_ray.dir_x, shadow_ray.dir_y, shadow_ray.dir_z}) ||
                    shadow_ray.tfar <= 0.0f) {
                    continue;
                }

                static std::atomic<bool> debug_shadow_logged{false};
                if (!debug_shadow_logged.exchange(true)) {
                    std::cerr << "[RayTracer] shadow org=("
                              << shadow_ray.org_x << "," << shadow_ray.org_y << "," << shadow_ray.org_z
                              << ") dir=(" << shadow_ray.dir_x << "," << shadow_ray.dir_y << "," << shadow_ray.dir_z
                              << ") tfar=" << shadow_ray.tfar << "\n";
                }

                RTCOccludedArguments o_args;
                rtcInitOccludedArguments(&o_args);
            rtcOccluded1(scene_, (RTCRay*)&shadow_ray, &o_args);

            if (shadow_ray.tfar < 0.0f) {
                in_shadow = true;
            }
        }

        if (!in_shadow) {
            math::Vector3 light_rgb = {
                light.color.r / 255.0f,
                light.color.g / 255.0f,
                light.color.b / 255.0f
            };

            // Simple Lambertian BRDF for direct lighting
            float nl = std::clamp(math::Vector3::dot(world_normal, light_dir), 0.0f, 1.0f);
            math::Vector3 diffuse = albedo * (1.0f - metallic) * nl;
            total_radiance += diffuse * light_rgb * light.intensity * attenuation;
        }
    }

    // Indirect lighting - simple bounce (diffuse only for minimal correctness)
    if (depth < kMaxBounces) {
        // Fresnel for specular
        math::Vector3 f0 = math::Vector3{0.04f, 0.04f, 0.04f} * (1.0f - metallic) + albedo * metallic;
        float specular_weight = fresnel_schlick(n_dot_v, f0).x;

        // Specular bounce
        if (specular_weight > 0.01f && roughness < 0.9f) {
            math::Vector3 reflect_dir = (direction - world_normal * 2.0f * math::Vector3::dot(direction, world_normal)).normalized();
            if (roughness > 0.05f) {
                reflect_dir = importance_sample_ggx(dist(rng), dist(rng), world_normal, roughness);
            }
            if (is_finite_vector3(reflect_dir) && reflect_dir.length_squared() > kTextureEpsilon &&
                math::Vector3::dot(reflect_dir, world_normal) > 0.0f) {
                ShadingResult spec_result = trace_ray(hit_pos + world_normal * 0.001f, reflect_dir, scene, rng, depth + 1, time);
                total_radiance += spec_result.color * fresnel_schlick(n_dot_v, f0) * specular_weight;
            }
        }

        // Diffuse bounce (one bounce only for minimal correctness)
        if (depth < 1) {
            float diffuse_weight = 1.0f - specular_weight;
            if (diffuse_weight > 0.01f) {
                // Cosine-weighted hemisphere sampling
                float r1 = dist(rng), r2 = dist(rng);
                float phi = 2.0f * PI * r1;
                float sqrt_r2 = std::sqrt(r2);
                math::Vector3 tangent, bitangent;
                if (std::abs(world_normal.x) > 0.1f) {
                    tangent = math::Vector3::cross(math::Vector3{0.0f, 1.0f, 0.0f}, world_normal).normalized();
                } else {
                    tangent = math::Vector3::cross(math::Vector3{1.0f, 0.0f, 0.0f}, world_normal).normalized();
                }
                bitangent = math::Vector3::cross(world_normal, tangent).normalized();

                math::Vector3 diffuse_dir = tangent * sqrt_r2 * std::cos(phi) +
                                          bitangent * sqrt_r2 * std::sin(phi) +
                                          world_normal * std::sqrt(1.0f - r2);
                diffuse_dir = diffuse_dir.normalized();

                if (is_finite_vector3(diffuse_dir) && diffuse_dir.length_squared() > kTextureEpsilon &&
                    math::Vector3::dot(diffuse_dir, world_normal) > 0.0f) {
                    ShadingResult diff_result = trace_ray(hit_pos + world_normal * 0.001f, diffuse_dir, scene, rng, depth + 1, time);
                    total_radiance += diff_result.color * albedo * diffuse_weight / PI;
                }
            }
        }
    }

    // Add emission
    math::Vector3 emission_color = {
        mat.emission_color.r / 255.0f,
        mat.emission_color.g / 255.0f,
        mat.emission_color.b / 255.0f
    };
    total_radiance += emission_color * mat.emission_strength;

    // Motion vector
    math::Vector2 motion_vector = {0.0f, 0.0f};
    if (hit_instance && hit_instance->previous_world_transform.has_value()) {
        // Transform hit_pos to object space, then to previous frame world space
        math::Matrix4x4 inv_curr_xform = hit_instance->world_transform.inverse_affine();
        math::Vector3 obj_pos = inv_curr_xform.transform_point(hit_pos);
        math::Vector3 prev_hit_pos = hit_instance->previous_world_transform->transform_point(obj_pos);
        
        // Motion vector: difference between current and previous world positions
        math::Vector3 delta = prev_hit_pos - hit_pos;
        motion_vector = math::Vector2{delta.x, delta.y};
    }

    ShadingResult result;
    result.color = total_radiance;
    result.alpha = mat.opacity;
    result.depth = rh.ray.tfar;
    result.normal = world_normal;
    result.albedo = albedo;
    result.motion_vector = motion_vector;
    result.object_id = hit_instance->object_id;
    result.material_id = hit_instance->material_id;
    return result;
}

} // namespace tachyon::renderer3d
