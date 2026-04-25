#include "tachyon/renderer2d/evaluated_composition/rendering/media_card_mesh_builder.h"

#include "tachyon/core/scene/evaluator/hashing.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tachyon::renderer2d {
namespace {

MediaCardMeshBuildResult make_card_mesh(
    const scene::EvaluatedLayerState& layer,
    const std::string& cache_prefix,
    const std::string& cache_key_seed,
    const SurfaceRGBA* surface) {

    auto mesh = std::make_shared<::tachyon::media::MeshAsset>();
    mesh->path = "inline:" + cache_prefix + ":" + layer.id;

    ::tachyon::media::MeshAsset::SubMesh submesh;
    submesh.vertices.resize(4);
    submesh.indices = {0U, 1U, 2U, 0U, 2U, 3U};

    const float half_w = std::max(1.0f, static_cast<float>(layer.width) * 0.5f);
    const float half_h = std::max(1.0f, static_cast<float>(layer.height) * 0.5f);

    submesh.vertices[0].position = {-half_w, -half_h, 0.0f};
    submesh.vertices[1].position = { half_w, -half_h, 0.0f};
    submesh.vertices[2].position = { half_w,  half_h, 0.0f};
    submesh.vertices[3].position = {-half_w,  half_h, 0.0f};

    submesh.vertices[0].normal = {0.0f, 0.0f, 1.0f};
    submesh.vertices[1].normal = {0.0f, 0.0f, 1.0f};
    submesh.vertices[2].normal = {0.0f, 0.0f, 1.0f};
    submesh.vertices[3].normal = {0.0f, 0.0f, 1.0f};

    submesh.vertices[0].uv = {0.0f, 0.0f};
    submesh.vertices[1].uv = {1.0f, 0.0f};
    submesh.vertices[2].uv = {1.0f, 1.0f};
    submesh.vertices[3].uv = {0.0f, 1.0f};

    submesh.material.base_color_factor = {1.0f, 1.0f, 1.0f};
    submesh.material.roughness_factor = 0.75f;
    submesh.material.metallic_factor = 0.0f;

    if (surface != nullptr) {
        ::tachyon::media::TextureData texture;
        texture.width = static_cast<int>(surface->width());
        texture.height = static_cast<int>(surface->height());
        texture.channels = 4;
        texture.data.resize(static_cast<std::size_t>(texture.width) * static_cast<std::size_t>(texture.height) * 4U);

        const auto& pixels = surface->pixels();
        for (std::size_t i = 0; i + 3 < pixels.size(); i += 4U) {
            const std::size_t out = i;
            texture.data[out + 0U] = static_cast<std::uint8_t>(std::clamp(pixels[i + 0U], 0.0f, 1.0f) * 255.0f);
            texture.data[out + 1U] = static_cast<std::uint8_t>(std::clamp(pixels[i + 1U], 0.0f, 1.0f) * 255.0f);
            texture.data[out + 2U] = static_cast<std::uint8_t>(std::clamp(pixels[i + 2U], 0.0f, 1.0f) * 255.0f);
            texture.data[out + 3U] = static_cast<std::uint8_t>(std::clamp(pixels[i + 3U], 0.0f, 1.0f) * 255.0f);
        }

        mesh->textures.push_back(std::move(texture));
        submesh.material.base_color_texture_idx = 0;
    } else {
        submesh.material.base_color_factor = {
            static_cast<float>(layer.fill_color.r) / 255.0f,
            static_cast<float>(layer.fill_color.g) / 255.0f,
            static_cast<float>(layer.fill_color.b) / 255.0f
        };
    }

    mesh->sub_meshes.push_back(std::move(submesh));

    MediaCardMeshBuildResult result;
    result.mesh = std::move(mesh);
    result.cache_key = "card3d:" + cache_prefix + ":" + layer.id + ":" + cache_key_seed;
    return result;
}

} // namespace

MediaCardMeshBuildResult build_colored_card_mesh(
    const scene::EvaluatedLayerState& layer,
    const std::string& cache_key_seed) {
    return make_card_mesh(layer, "solid", cache_key_seed, nullptr);
}

MediaCardMeshBuildResult build_textured_card_mesh(
    const scene::EvaluatedLayerState& layer,
    const SurfaceRGBA& surface,
    const std::string& cache_key_seed) {
    return make_card_mesh(layer, "media", cache_key_seed, &surface);
}

} // namespace tachyon::renderer2d
