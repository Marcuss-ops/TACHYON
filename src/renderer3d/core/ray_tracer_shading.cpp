#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/materials/pbr_utils.h"
#include "tachyon/core/math/matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace tachyon::renderer3d {

using namespace pbr;

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

    // Embree ray hit structure
    RTCRayHit rh{};
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

    // Miss - sample environment
    if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        ShadingResult env_result;
        if (current_env_map_) {
            EnvironmentSample env_sample = environment_manager_.sample_environment(
                direction, current_env_map_, environment_intensity_, environment_rotation_);
            env_result.color = env_sample.color;
            env_result.alpha = env_sample.alpha;
            env_result.depth = env_sample.depth;
            env_result.normal = env_sample.normal;
            env_result.albedo = env_sample.albedo;
        } else {
            env_result.color = math::Vector3{0.2f, 0.2f, 0.2f};
            env_result.alpha = 0.0f;
            env_result.depth = 1e6f;
            env_result.normal = math::Vector3{0.0f, 0.0f, 1.0f};
            env_result.albedo = math::Vector3{0.0f, 0.0f, 0.0f};
        }
        env_result.motion_vector = math::Vector2{0.0f, 0.0f};
        env_result.object_id = 0;
        env_result.material_id = 0;
        return env_result;
    }

    // Hit - find instance
    const GeoInstance* hit_instance = nullptr;
    for (const auto& inst : instances_) {
        if (inst.geom_id == rh.hit.geomID) {
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

    // Compute hit position and normal
    math::Vector3 normal = {rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z};
    normal = normal.normalized();

    // Transform normal using the inverse-transpose of the world transform.
    const math::Matrix4x4 normal_matrix = hit_instance->world_transform.inverse_affine();
    math::Vector3 world_normal = {
        normal_matrix[0] * normal.x + normal_matrix[4] * normal.y + normal_matrix[8] * normal.z,
        normal_matrix[1] * normal.x + normal_matrix[5] * normal.y + normal_matrix[9] * normal.z,
        normal_matrix[2] * normal.x + normal_matrix[6] * normal.y + normal_matrix[10] * normal.z
    };
    world_normal = world_normal.normalized();

    math::Vector3 hit_pos = origin + direction * rh.ray.tfar;

    // Use EvaluatedMaterial directly (not through MaterialEvaluator)
    const EvaluatedMaterial& mat = hit_instance->material;

    // Simple material evaluation
    math::Vector3 albedo = {
        mat.base_color.r / 255.0f,
        mat.base_color.g / 255.0f,
        mat.base_color.b / 255.0f
    };
    float metallic = std::clamp(mat.metallic, 0.0f, 1.0f);
    float roughness = std::max(mat.roughness, 0.05f);

    math::Vector3 view_dir = (-direction).normalized();
    float n_dot_v = std::clamp(math::Vector3::dot(world_normal, view_dir), 0.0f, 1.0f);

    math::Vector3 total_radiance = {0.0f, 0.0f, 0.0f};

    // Direct lighting
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (const auto& light : scene.lights) {
        math::Vector3 light_dir;
        float distance = INFINITY;
        float attenuation = 1.0f;

        if (light.type == LightType::Ambient) {
            total_radiance += albedo * light.intensity * 0.1f;
            continue;
        } else if (light.type == LightType::Directional) {
            light_dir = (-light.direction).normalized();
        } else {
            math::Vector3 light_pos = light.position;
            math::Vector3 delta = light_pos - hit_pos;
            distance = delta.length();
            light_dir = delta / distance;

            // Attenuation (simple inverse square for now)
            if (distance > 0.0f) {
                attenuation = 1.0f / (distance * distance);
            }

            // Spotlight
            if (light.type == LightType::Spot) {
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
            if (math::Vector3::dot(reflect_dir, world_normal) > 0.0f) {
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

                if (math::Vector3::dot(diffuse_dir, world_normal) > 0.0f) {
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
