#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/renderer2d/core/texture_handle.h"
#include <vector>
#include <cstdint>

namespace tachyon {
namespace renderer3d {

struct Vertex3D {
    math::Vector3 position;
    math::Vector2 uv;
    math::Vector3 normal;
};

struct Material3D {
    float metallic{0.0f};
    float roughness{0.5f};
    float ior{1.45f};
    float transmission{0.0f};
    float emission_strength{0.0f};
    math::Vector3 emission_color{1.0f, 1.0f, 1.0f};
};

struct Mesh3D {
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    renderer2d::TextureHandle texture;
    Material3D material;
};

} // namespace renderer3d
} // namespace tachyon
