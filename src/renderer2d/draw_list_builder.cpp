#include "tachyon/renderer2d/draw_list_builder.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace tachyon {
namespace renderer2d {

std::vector<DrawCommand2D> map_layer_to_draw_commands(
    const timeline::EvaluatedLayerState& layer,
    const timeline::EvaluatedCompositionState& composition_state,
    int z_order
);

DrawList2D DrawListBuilder::build(const timeline::EvaluatedCompositionState& composition_state) {
    DrawList2D draw_list;

    DrawCommand2D clear_command;
    clear_command.kind = DrawCommandKind::Clear;
    clear_command.z_order = -1;
    clear_command.blend_mode = BlendMode::Normal;
    clear_command.clear = ClearCommand{Color::black()};
    draw_list.commands.push_back(clear_command);

    for (std::size_t index = 0; index < composition_state.layers.size(); ++index) {
        auto layer_commands = map_layer_to_draw_commands(
            composition_state.layers[index],
            composition_state,
            static_cast<int>(index)
        );
        draw_list.commands.insert(draw_list.commands.end(), layer_commands.begin(), layer_commands.end());
    }

    std::stable_sort(draw_list.commands.begin(), draw_list.commands.end(), [](const DrawCommand2D& a, const DrawCommand2D& b) {
        return a.z_order < b.z_order;
    });

    return draw_list;
}

} // namespace renderer2d
} // namespace tachyon
