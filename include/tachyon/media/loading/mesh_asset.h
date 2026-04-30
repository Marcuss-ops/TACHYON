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

struct PreFilteredEnvMap {
    struct Level {
        std::vector<float> data;
        int width{0};
        int height{0};
    };
    std::vector<Level> levels; // Mip-like levels for different roughness
};

struct BRDFLut {
    std::vector<float> data;
    int size{0};
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
        std::uint32_t joints[4]{0, 0, 0, 0};
        float weights[4]{0.0f, 0.0f, 0.0f, 0.0f};
    };

    struct MorphTarget {
        std::vector<math::Vector3> positions;
        std::vector<math::Vector3> normals;
    };

    struct SubMesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        MaterialData material;
        math::Matrix4x4 transform{math::Matrix4x4::identity()};
        std::vector<MorphTarget> morph_targets;
    };

    struct Joint {
        std::string name;
        int index{-1};
        math::Matrix4x4 inverse_bind_matrix{math::Matrix4x4::identity()};
        math::Matrix4x4 local_transform{math::Matrix4x4::identity()};
        int parent{-1};
        std::vector<int> children;
    };

    struct Skin {
        std::string name;
        std::vector<Joint> joints;
    };

    struct AnimationChannel {
        enum class Path { Translation, Rotation, Scale, Weights };
        Path path;
        int node_idx{-1};
        std::vector<float> times;
        std::vector<float> values; // Float4 for rotation, float3 for others
    };

    struct Animation {
        std::string name;
        std::vector<AnimationChannel> channels;
    };

    std::string path;
    std::vector<SubMesh> sub_meshes;
    std::vector<TextureData> textures;
    std::vector<Skin> skins;
    std::vector<Animation> animations;

    bool empty() const { return sub_meshes.empty(); }
};

} // namespace tachyon::media
