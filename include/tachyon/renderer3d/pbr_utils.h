#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/media/mesh_asset.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer3d::pbr {

const float PI = 3.1415926535f;

math::Vector3 fresnel_schlick(float cos_theta, const math::Vector3& f0);
float ndf_ggx(float n_dot_h, float roughness);
float geometry_schlick_ggx(float n_dot_v, float roughness);
float geometry_smith(float n_dot_v, float n_dot_l, float roughness);
math::Vector2 sample_disk(float radius, float u1, float u2);
math::Vector3 importance_sample_ggx(float u1, float u2, const math::Vector3& N, float roughness);
math::Vector3 sample_equirect(const media::HDRTextureData& map, const math::Vector3& dir, float rotation);

} // namespace tachyon::renderer3d::pbr
