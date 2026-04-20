#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/matrix4x4.h"
#include <vector>
#include <string>

namespace tachyon::media {

struct TextureData {
    std::vector<unsigned char> data;
    int width{0};
    int height{0};
    int channels{0};
};

struct HDRTextureData {
    std::vector<float> data;
    int width{0};
    int height{0};
    int channels{0};
};

struct MaterialData {
    math::Vector3 base_color_factor{1.0f, 1.0f, 1.0f};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    
    int base_color_texture_idx{-1};
    int metallic_roughness_texture_idx{-1};
    int normal_texture_idx{-1};
};

struct MeshAsset {
    struct Vertex {
        math::Vector3 position;
        math::Vector3 normal;
        math::Vector2 uv;
    };

    struct SubMesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        MaterialData material;
        math::Matrix4x4 transform{math::Matrix4x4::identity()};
    };

    std::string path;
    std::vector<SubMesh> sub_meshes;
    std::vector<TextureData> textures;

    bool empty() const { return sub_meshes.empty(); }
};

} // namespace tachyon::media
