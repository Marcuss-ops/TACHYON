#pragma once

#include <optional>
#include <filesystem>

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/resource/render_context.h"

namespace tachyon {

/**
 * @brief Resolve the media source path for a layer.
 */
std::optional<std::filesystem::path> resolve_media_source(
    const scene::EvaluatedLayerState& layer,
    const renderer2d::RenderContext2D& context);

} // namespace tachyon
