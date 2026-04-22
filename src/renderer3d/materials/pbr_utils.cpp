#include "tachyon/renderer3d/materials/pbr_utils.h"
#include "tachyon/renderer3d/materials/material_evaluator.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer3d::pbr {

namespace {

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

math::Vector3 lerp(const math::Vector3& a, const math::Vector3& b, float t) {
    return a + (b - a) * t;
}

} // namespace

math::Vector3 fresnel_schlick(float cos_theta, const math::Vector3& f0) {
    return f0 + (math::Vector3{1.0f, 1.0f, 1.0f} - f0) * std::pow(std::clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float ndf_ggx(float n_dot_h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h2 = n_dot_h * n_dot_h;
    float denom = (n_dot_h2 * (a2 - 1.0f) + 1.0f);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    return n_dot_v / (n_dot_v * (1.0f - k) + k);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
    return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}

math::Vector2 sample_disk(float radius, float u1, float u2) {
    if (radius <= 0.0f) return {0.0f, 0.0f};
    float r = radius * std::sqrt(u1);
    float theta = 2.0f * PI * u2;
    return { r * std::cos(theta), r * std::sin(theta) };
}

math::Vector3 importance_sample_ggx(float u1, float u2, const math::Vector3& N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0f * PI * u1;
    float cos_theta = std::sqrt((1.0f - u2) / (1.0f + (a * a - 1.0f) * u2));
    float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

    math::Vector3 H;
    H.x = sin_theta * std::cos(phi);
    H.y = sin_theta * std::sin(phi);
    H.z = cos_theta;

    math::Vector3 up = std::abs(N.z) < 0.999f ? math::Vector3{0, 0, 1} : math::Vector3{1, 0, 0};
    math::Vector3 tangent = math::Vector3::cross(up, N).normalized();
    math::Vector3 bitangent = math::Vector3::cross(N, tangent);

    return (tangent * H.x + bitangent * H.y + N * H.z).normalized();
}

math::Vector3 sample_equirect(const media::HDRTextureData& map, const math::Vector3& dir, float rotation) {
    float phi = std::atan2(dir.z, dir.x) + rotation * (PI / 180.0f);
    float theta = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
    float u = 0.5f + phi / (2.0f * PI);
    float v = 0.5f - theta / PI;
    
    float fx = u * map.width - 0.5f;
    float fy = v * map.height - 0.5f;
    int ix = static_cast<int>(std::floor(fx));
    int iy = static_cast<int>(std::floor(fy));
    float tx = fx - ix;
    float ty = fy - iy;

    auto get_px = [&](int x, int y) -> math::Vector3 {
        x = (x + map.width) % map.width;
        y = std::clamp(y, 0, map.height - 1);
        int idx = (y * map.width + x) * map.channels;
        return { map.data[idx+0] / 255.0f, map.data[idx+1] / 255.0f, map.data[idx+2] / 255.0f };
    };

    math::Vector3 p00 = get_px(ix, iy);
    math::Vector3 p10 = get_px(ix + 1, iy);
    math::Vector3 p01 = get_px(ix, iy + 1);
    math::Vector3 p11 = get_px(ix + 1, iy + 1);

    math::Vector3 p0 = p00 * (1.0f - tx) + p10 * tx;
    math::Vector3 p1 = p01 * (1.0f - tx) + p11 * tx;
    return p0 * (1.0f - ty) + p1 * ty;
}

math::Vector3 evaluate_disney_bsdf(
    const ::tachyon::renderer3d::MaterialSample& mat,
    const math::Vector3& N,
    const math::Vector3& V,
    const math::Vector3& L) 
{
    const math::Vector3 H = (V + L).normalized();
    const float NdotL = std::clamp(math::Vector3::dot(N, L), 0.0f, 1.0f);
    const float NdotV = std::clamp(math::Vector3::dot(N, V), 0.0f, 1.0f);
    if (NdotL <= 0.0f || NdotV <= 0.0f) return math::Vector3::zero();

    const float LdotH = std::clamp(math::Vector3::dot(L, H), 0.0f, 1.0f);
    const float NdotH = std::clamp(math::Vector3::dot(N, H), 0.0f, 1.0f);

    const math::Vector3 Cdlin = mat.base_color;
    const float Cdlum = 0.3f * Cdlin.x + 0.6f * Cdlin.y + 0.1f * Cdlin.z; // Tint luminance
    const math::Vector3 Ctint = Cdlum > 0.0f ? Cdlin / Cdlum : math::Vector3{1, 1, 1}; // Tint color
    const math::Vector3 Cspec0 = lerp(mat.specular * 0.08f * lerp(math::Vector3{1,1,1}, Ctint, mat.specular_tint), Cdlin, mat.metallic);
    const math::Vector3 Csheen = lerp(math::Vector3{1,1,1}, Ctint, mat.sheen_tint);

    // Diffuse lobe
    const float FL = std::pow(1.0f - NdotL, 5.0f);
    const float FV = std::pow(1.0f - NdotV, 5.0f);
    const float RR = 2.0f * mat.roughness * LdotH * LdotH;
    const float Fd90 = 0.5f + RR;
    const float Fd = lerp(1.0f, Fd90, FL) * lerp(1.0f, Fd90, FV);
    
    // Subsurface lobe (Burley-style approximation).
    const float Fss90 = LdotH * LdotH * mat.roughness;
    const float Fss = lerp(1.0f, Fss90, FL) * lerp(1.0f, Fss90, FV);
    const float ss = 1.25f * (Fss * (1.0f / (NdotL + NdotV) - 0.5f) + 0.5f);
    
    const math::Vector3 diffuse = (1.0f / PI) * lerp(Fd, ss, mat.subsurface) * Cdlin * (1.0f - mat.metallic);

    // Specular lobe (GGX)
    const float D = ndf_ggx(NdotH, mat.roughness);
    const float G = geometry_smith(NdotV, NdotL, mat.roughness);
    const math::Vector3 F = fresnel_schlick(LdotH, Cspec0);
    const math::Vector3 specular = D * G * F / (4.0f * NdotL * NdotV + 0.0001f);

    // Sheen lobe
    const float FH = std::pow(1.0f - LdotH, 5.0f);
    const math::Vector3 sheen = FH * mat.sheen * Csheen;

    // Clearcoat lobe
    const float Dcc = ndf_ggx(NdotH, mat.clearcoat_roughness); 
    const float Gcc = geometry_smith(NdotV, NdotL, 0.25f);
    const float Fcc = lerp(0.04f, 1.0f, FH);
    const math::Vector3 clearcoat = math::Vector3{1,1,1} * (0.25f * mat.clearcoat * Dcc * Gcc * Fcc / (4.0f * NdotL * NdotV + 0.0001f));

    return (diffuse + sheen) * (math::Vector3{1,1,1} - F) + specular + clearcoat;
}

} // namespace tachyon::renderer3d::pbr
