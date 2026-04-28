#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include "tachyon/text/animation/text_animation_options.h"
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
    const ::tachyon::text::FontRegistry& font_registry,
    const ::tachyon::text::TextAnimationOptions& animation = {});

} // namespace tachyon::renderer2d

