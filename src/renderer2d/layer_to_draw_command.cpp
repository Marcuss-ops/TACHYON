#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/scene/evaluated_state.h"

#include <vector>

namespace tachyon {
namespace renderer2d {

std::vector<DrawCommand2D> map_layer_to_draw_commands(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& composition_state,
    int z_order
);

} // namespace renderer2d
} // namespace tachyon
