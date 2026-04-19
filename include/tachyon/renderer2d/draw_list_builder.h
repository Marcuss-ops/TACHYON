#pragma once

#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/timeline/evaluated_state.h"

namespace tachyon {
namespace renderer2d {

class DrawListBuilder {
public:
    [[nodiscard]] static DrawList2D build(const timeline::EvaluatedCompositionState& composition_state);
};

} // namespace renderer2d
} // namespace tachyon
