#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/text/fonts/font_registry.h"
#include "tachyon/media/loading/mesh_asset.h"

#include <memory>
#include <string>

namespace tachyon::renderer2d {

struct TextMeshBuildResult {
    std::shared_ptr<const ::tachyon::media::MeshAsset> mesh;
    std::string cache_key;
};

[[nodiscard]] TextMeshBuildResult build_text_extrusion_mesh(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& composition,
    const ::tachyon::text::FontRegistry& font_registry);

} // namespace tachyon::renderer2d
