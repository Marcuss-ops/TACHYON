#pragma once

#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/core/scene/evaluated_state.h"

namespace tachyon {
namespace renderer2d {

class DrawListBuilder {
public:
    [[nodiscard]] static DrawList2D build(const scene::EvaluatedCompositionState& composition_state);
};

} // namespace renderer2d
} // namespace tachyon
