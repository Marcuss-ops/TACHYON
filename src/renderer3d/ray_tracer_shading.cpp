#include "tachyon/renderer3d/ray_tracer.h"
#include "tachyon/renderer3d/pbr_utils.h"
#include "tachyon/core/math/matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace tachyon::renderer3d {

using namespace pbr;

ShadingResult RayTracer::trace_ray(
    const math::Vector3& origin,
    const math::Vector3& direction,
    const scene::EvaluatedCompositionState& state,
    std::mt19937& rng,
    int depth) 
{
    if (depth > kMaxBounces) return {{0,0,0}, 0.0f, 0.0f, {0,0,0}, {0,0,1}};

    RTCRayHit rh{};
    rh.ray.org_x = origin.x; rh.ray.org_y = origin.y; rh.ray.org_z = origin.z;
    rh.ray.dir_x = direction.x; rh.ray.dir_y = direction.y; rh.ray.dir_z = direction.z;
    rh.ray.tnear = 0.001f; // Avoid self-intersection
    rh.ray.tfar = 100000.0f;
    rh.ray.mask = static_cast<unsigned int>(-1);
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    rtcIntersect1(scene_, &rh, &args);

    if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return environment_manager_.sample_environment(direction, environment_map_, environment_intensity_, environment_rotation_);
    }

    const scene::EvaluatedLayerState* layer = nullptr;
    const media::MeshAsset* parent_asset = nullptr;
    const media::MeshAsset::SubMesh* sub_mesh = nullptr;
    for (const auto& inst : instances_) {
        if (inst.geom_id == rh.hit.geomID) {
            layer = inst.layer_ref;
            parent_asset = inst.parent_asset;
            sub_mesh = inst.sub_mesh;
            break;
        }
    }

    if (!layer) return {{1, 0, 1}, 1.0f, rh.ray.tfar, {1,0,1}, {0,0,1}}; // Magenta error

    float u = rh.hit.u, v = rh.hit.v, w = 1.0f - u - v;
    math::Vector3 normal = {rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z};
    normal = normal.normalized();
    
    if (sub_mesh && !sub_mesh->vertices.empty()) {
        unsigned int prim_id = rh.hit.primID;
        unsigned int i0 = sub_mesh->indices[prim_id * 3 + 0], i1 = sub_mesh->indices[prim_id * 3 + 1], i2 = sub_mesh->indices[prim_id * 3 + 2];
        normal = (sub_mesh->vertices[i0].normal * w + sub_mesh->vertices[i1].normal * u + sub_mesh->vertices[i2].normal * v).normalized();
    }

    math::Vector3 view_vec = -direction;
    float n_dot_v = std::clamp(static_cast<float>(math::Vector3::dot(normal, view_vec)), 0.0f, 1.0f);
    math::Vector3 hit_pos = origin + direction * rh.ray.tfar;

    math::Vector2 uv{0, 0};
    if (sub_mesh) {
        unsigned int prim_id = rh.hit.primID;
        unsigned int i0 = sub_mesh->indices[prim_id * 3 + 0], i1 = sub_mesh->indices[prim_id * 3 + 1], i2 = sub_mesh->indices[prim_id * 3 + 2];
        uv = sub_mesh->vertices[i0].uv * w + sub_mesh->vertices[i1].uv * u + sub_mesh->vertices[i2].uv * v;
    } else {
        if (rh.hit.primID == 0) uv = math::Vector2{0,0} * (1-u-v) + math::Vector2{1,0} * u + math::Vector2{1,1} * v;
        else uv = math::Vector2{0,0} * (1-u-v) + math::Vector2{1,1} * u + math::Vector2{0,1} * v;
    }
    
    math::Vector3 albedo = {layer->fill_color.r / 255.0f, layer->fill_color.g / 255.0f, layer->fill_color.b / 255.0f};
    float metallic = layer->material.metallic, roughness = std::max(layer->material.roughness, 0.05f);

    if (sub_mesh) {
        albedo = albedo * sub_mesh->material.base_color_factor;
        metallic = metallic * sub_mesh->material.metallic_factor;
        roughness = std::max(roughness * sub_mesh->material.roughness_factor, 0.05f);
        if (sub_mesh->material.base_color_texture_idx >= 0 && parent_asset && sub_mesh->material.base_color_texture_idx < (int)parent_asset->textures.size()) {
            const auto& tex = parent_asset->textures[sub_mesh->material.base_color_texture_idx];
            if (!tex.data.empty() && tex.width > 0 && tex.height > 0 && tex.channels > 0) {
                int tx = std::clamp((int)((uv.x - std::floor(uv.x)) * tex.width), 0, tex.width - 1);
                int ty = std::clamp((int)((uv.y - std::floor(uv.y)) * tex.height), 0, tex.height - 1);
                int idx = (ty * tex.width + tx) * tex.channels;
                albedo = albedo * math::Vector3{ tex.data[idx+0]/255.0f, tex.data[idx+1]/255.0f, tex.data[idx+2]/255.0f };
            }
        }
        if (sub_mesh->material.metallic_roughness_texture_idx >= 0 && parent_asset && sub_mesh->material.metallic_roughness_texture_idx < (int)parent_asset->textures.size()) {
            const auto& tex = parent_asset->textures[sub_mesh->material.metallic_roughness_texture_idx];
            if (!tex.data.empty() && tex.width > 0 && tex.height > 0 && tex.channels >= 3) {
                int tx = std::clamp((int)((uv.x - std::floor(uv.x)) * tex.width), 0, tex.width - 1);
                int ty = std::clamp((int)((uv.y - std::floor(uv.y)) * tex.height), 0, tex.height - 1);
                int idx = (ty * tex.width + tx) * tex.channels;
                roughness = std::max(roughness * (tex.data[idx+1]/255.0f), 0.05f); metallic = metallic * (tex.data[idx+2]/255.0f);
            }
        }
    } else if (layer->texture_rgba && layer->width > 0 && layer->height > 0) {
        int tx = std::clamp((int)((uv.x - std::floor(uv.x)) * layer->width), 0, (int)layer->width - 1);
        int ty = std::clamp((int)((uv.y - std::floor(uv.y)) * layer->height), 0, (int)layer->height - 1);
        const uint8_t* tex_ptr = layer->texture_rgba + (ty * layer->width + tx) * 4;
        albedo = { tex_ptr[0] / 255.0f, tex_ptr[1] / 255.0f, tex_ptr[2] / 255.0f };
    }

    math::Vector3 f0 = math::Vector3{0.04f, 0.04f, 0.04f} * (1.0f - metallic) + albedo * metallic;
    math::Vector3 total_lighting = {0.0f, 0.0f, 0.0f};
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (const auto& light : state.lights) {
        math::Vector3 l_dir; float distance = INFINITY, attenuation = 1.0f;
        if (light.type == "ambient") { total_lighting += albedo * light.color.to_vector3() * light.intensity * 0.1f; continue; }
        else if (light.type == "parallel") l_dir = -light.direction;
        else {
            math::Vector3 light_pos = light.position;
            if (light.shadow_radius > 0.0f) {
                math::Vector2 lo = sample_disk(light.shadow_radius, dist(rng), dist(rng));
                math::Vector3 lsd = light.direction.length_squared() > 1e-6f ? light.direction : math::Vector3{0, 0, -1};
                math::Vector3 lr = (std::abs(lsd.y) > 0.99f) ? math::Vector3{1,0,0} : math::Vector3{0,1,0};
                math::Vector3 lu = math::Vector3::cross(lsd, lr).normalized(); lr = math::Vector3::cross(lu, lsd).normalized();
                light_pos += lr * lo.x + lu * lo.y;
            }
            math::Vector3 delta = light_pos - hit_pos; distance = delta.length(); l_dir = delta / distance;
            if (distance > 0.0f) {
                if (light.falloff_type == "inverse_square") attenuation = 1.0f / (distance * distance);
                else if (light.falloff_type == "linear") attenuation = std::clamp(1.0f - (distance - light.attenuation_near) / (light.attenuation_far - light.attenuation_near), 0.0f, 1.0f);
                else { float d = std::clamp((distance - light.attenuation_near) / (light.attenuation_far - light.attenuation_near), 0.0f, 1.0f); attenuation = 1.0f - d * d * (3.0f - 2.0f * d); }
            }
            if (light.type == "spot") {
                float cos_theta = math::Vector3::dot(l_dir, -light.direction), fa = light.cone_angle * pbr::PI / 180.0f;
                float co = std::cos(fa * 0.5f), ci = std::cos(std::max(0.0f, fa * 0.5f - (light.cone_feather / 100.0f) * fa * 0.5f));
                attenuation *= std::clamp((cos_theta - co) / (ci - co), 0.0f, 1.0f);
            }
        }
        if (attenuation <= 0.0f) continue;
        bool in_shadow = false;
        if (light.casts_shadows) {
            alignas(16) RTCRay shadow_ray{}; shadow_ray.org_x = hit_pos.x + normal.x * 0.01f; shadow_ray.org_y = hit_pos.y + normal.y * 0.01f; shadow_ray.org_z = hit_pos.z + normal.z * 0.01f;
            shadow_ray.dir_x = l_dir.x; shadow_ray.dir_y = l_dir.y; shadow_ray.dir_z = l_dir.z; shadow_ray.tnear = 0.0f; shadow_ray.tfar = distance - 0.02f; shadow_ray.mask = static_cast<unsigned int>(-1);
            RTCOccludedArguments o_args; rtcInitOccludedArguments(&o_args); rtcOccluded1(scene_, (::RTCRay*)&shadow_ray, &o_args);
            if (shadow_ray.tfar < 0.0f) in_shadow = true;
        }
        if (!in_shadow) {
            math::Vector3 h = (view_vec + l_dir).normalized();
            float nl = std::clamp(math::Vector3::dot(normal, l_dir), 0.0f, 1.0f), nh = std::clamp(math::Vector3::dot(normal, h), 0.0f, 1.0f), vh = std::clamp(math::Vector3::dot(view_vec, h), 0.0f, 1.0f);
            float d = ndf_ggx(nh, roughness), g = geometry_smith(n_dot_v, nl, roughness);
            math::Vector3 ks = fresnel_schlick(vh, f0), kd = (math::Vector3{1,1,1} - ks) * (1.0f - metallic);
            math::Vector3 light_rgb = {light.color.r / 255.0f, light.color.g / 255.0f, light.color.b / 255.0f};
            total_lighting += (kd * albedo * (1.0f / pbr::PI) + ks * (d * g / (4.0f * n_dot_v * nl + 0.001f))) * light_rgb * (light.intensity * attenuation) * nl;
        }
    }

    if (depth < kMaxBounces) {
        float refl = (metallic < 0.01f) ? fresnel_schlick(n_dot_v, {0.04f, 0.04f, 0.04f}).x : metallic;
        if (refl > 0.01f) {
            math::Vector3 rd = (roughness < 0.05f) ? (direction - normal * 2.0f * math::Vector3::dot(direction, normal)).normalized() : (direction - importance_sample_ggx(dist(rng), dist(rng), normal, roughness) * 2.0f * math::Vector3::dot(direction, importance_sample_ggx(dist(rng), dist(rng), normal, roughness))).normalized();
            if (math::Vector3::dot(rd, normal) > 0.0f) total_lighting += fresnel_schlick(std::clamp(math::Vector3::dot(view_vec, normal), 0.0f, 1.0f), f0) * trace_ray(hit_pos + normal * 0.001f, rd, state, rng, depth + 1).color;
        }
        float trans = 1.0f - (float)layer->opacity;
        if (trans > 0.01f) {
            float eta = 1.0f / layer->material.ior; math::Vector3 n = normal; if (math::Vector3::dot(direction, n) > 0) { n = -normal; eta = layer->material.ior; }
            float ci = -math::Vector3::dot(direction, n), s2t = eta * eta * (1.0f - ci * ci);
            if (s2t <= 1.0f) total_lighting = total_lighting * (1.0f - trans) + trace_ray(hit_pos - n * 0.001f, (direction * eta + n * (eta * ci - std::sqrt(1.0f - s2t))).normalized(), state, rng, depth + 1).color * trans;
        }
        if (trans < 0.9f && metallic < 0.9f) {
            float r1 = dist(rng), r2 = dist(rng), phi = 2.0f * pbr::PI * r1, r = std::sqrt(r2);
            math::Vector3 ld = { r * std::cos(phi), r * std::sin(phi), std::sqrt(1.0f - r2) }, t = (std::abs(normal.x) > 0.1f) ? math::Vector3{0, 1, 0} : math::Vector3{1, 0, 0}, bt = math::Vector3::cross(normal, t).normalized(); t = math::Vector3::cross(bt, normal).normalized();
            total_lighting += (math::Vector3{1,1,1} - fresnel_schlick(n_dot_v, f0)) * (1.0f - metallic) * albedo * trace_ray(hit_pos + normal * 0.001f, t * ld.x + bt * ld.y + normal * ld.z, state, rng, depth + 1).color;
        }
    } else if (environment_map_) {
        math::Vector3 ks = fresnel_schlick(n_dot_v, f0), kd = (math::Vector3{1,1,1} - ks) * (1.0f - metallic);
        math::Vector3 de = environment_manager_.sample_environment(normal, environment_map_, environment_intensity_, environment_rotation_, 1.0f).color;
        math::Vector3 se = environment_manager_.sample_environment((direction - normal * 2.0f * math::Vector3::dot(direction, normal)).normalized(), environment_map_, environment_intensity_, environment_rotation_, roughness).color;
        const media::BRDFLut* lut = EnvironmentManager::get_brdf_lut(); float s = 1.0f, b = 0.0f;
        if (lut && lut->size > 0) { int lx = std::clamp((int)(roughness * (lut->size - 1)), 0, lut->size - 1), ly = std::clamp((int)(n_dot_v * (lut->size - 1)), 0, lut->size - 1); s = lut->data[(ly * lut->size + lx) * 2 + 0]; b = lut->data[(ly * lut->size + lx) * 2 + 1]; }
        total_lighting += (albedo * (kd * de)) + (se * (ks * s + math::Vector3{b, b, b}));
    } else total_lighting += albedo * 0.03f;

    total_lighting += albedo * layer->material.emission;
    return {total_lighting, (float)layer->opacity, rh.ray.tfar, albedo, normal};
}

} // namespace tachyon::renderer3d
