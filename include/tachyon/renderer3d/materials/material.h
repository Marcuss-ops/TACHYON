#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/render/scene_3d.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/media/loading/mesh_asset.h"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon::renderer3d {

class MaterialSystem {
public:
    struct MaterialInputs {
        math::Vector3 base_color{1.0f, 1.0f, 1.0f};
        float metallic{0.0f};
        float roughness{0.5f};
        float ao{1.0f};
        float opacity{1.0f};
        float ior{1.45f};

        math::Vector3 emissive{0.0f, 0.0f, 0.0f};
        float emissive_intensity{0.0f};

        math::Vector3 normal{0.0f, 0.0f, 1.0f};

        bool has_normal_map{false};
        bool has_emissive_map{false};
        bool has_ao_map{false};
    };

    struct TextureSlot {
        int texture_index{-1};
        float scale_u{1.0f};
        float scale_v{1.0f};
        float offset_u{0.0f};
        float offset_v{0.0f};
        std::string wrap_mode{"repeat"};
        std::string filter_mode{"linear"};
    };

    struct MaterialDefinition {
        std::string name;
        std::string shader{"pbr"};

        TextureSlot base_color_map;
        TextureSlot metallic_roughness_map;
        TextureSlot normal_map;
        TextureSlot ao_map;
        TextureSlot emissive_map;
        TextureSlot occlusion_map;

        MaterialInputs defaults;
    };

    static MaterialInputs evaluate(
        const EvaluatedMeshInstance3D& instance,
        const media::MeshAsset* asset,
        const math::Vector2& uv);

    static MaterialInputs evaluate_with_textures(
        const EvaluatedMeshInstance3D& instance,
        const media::MeshAsset* asset,
        const media::MeshAsset::SubMesh* sub_mesh,
        const math::Vector2& uv);

    static math::Vector3 compute_normal(
        const math::Vector3& geometric_normal,
        const math::Vector3& tangent,
        const math::Vector3& bitangent,
        const math::Vector2& normal_map_value,
        float normal_strength);

    static float sample_metallic_from_map(const std::vector<uint8_t>& data, int width, int height, int channels, int x, int y);
    static math::Vector3 sample_normal_from_map(const std::vector<uint8_t>& data, int width, int height, int channels, int x, int y);
    static math::Vector3 sample_emissive_from_map(const std::vector<uint8_t>& data, int width, int height, int channels, int x, int y);
    static float sample_ao_from_map(const std::vector<uint8_t>& data, int width, int height, int channels, int x, int y);

    static math::Vector2 transform_uv(
        const math::Vector2& uv,
        const TextureSlot& slot);

    static void set_quality_tier(const std::string& tier);
    static std::string get_quality_tier();

    struct ProfileStats {
        int texture_samples{0};
        int normal_map_samples{0};
        int emissive_map_samples{0};
        double sample_time_ms{0.0};
    };

    static const ProfileStats& get_stats();
    static void reset_stats();

private:
    static std::string quality_tier_;
    static ProfileStats stats_;
};

} // namespace tachyon::renderer3d