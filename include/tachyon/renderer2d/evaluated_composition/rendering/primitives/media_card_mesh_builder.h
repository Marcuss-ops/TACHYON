#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/media/loading/mesh_asset.h"

#include <memory>
#include <string>

namespace tachyon::renderer2d {

struct MediaCardMeshBuildResult {
    std::shared_ptr<const ::tachyon::media::MeshAsset> mesh;
    std::string cache_key;
};

[[nodiscard]] MediaCardMeshBuildResult build_colored_card_mesh(
    const scene::EvaluatedLayerState& layer,
    const std::string& cache_key_seed = {});

[[nodiscard]] MediaCardMeshBuildResult build_textured_card_mesh(
    const scene::EvaluatedLayerState& layer,
    const SurfaceRGBA& surface,
    const std::string& cache_key_seed);

} // namespace tachyon::renderer2d
