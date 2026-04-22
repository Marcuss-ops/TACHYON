#include "tachyon/runtime/core/data/compiled_scene.h"

namespace tachyon {

bool CompiledScene::is_valid() const noexcept {
    if (header.magic != 0x54414348U) return false;
    if (header.version != 1) return false;
    
    // Validate layout integrity
    if (header.layout_checksum != calculate_layout_checksum()) return false;
    
    return true;
}

void CompiledScene::invalidate_node(std::uint32_t node_id) {
    std::vector<std::uint32_t> affected;
    graph.propagate_invalidation(node_id, affected);
    
    for (std::uint32_t id : affected) {
        bool found = false;
        for (auto& comp : compositions) {
            if (comp.node.node_id == id) {
                comp.node.version++;
                found = true;
                break;
            }
            for (auto& layer : comp.layers) {
                if (layer.node.node_id == id) {
                    layer.node.version++;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (found) continue;

        for (auto& track : property_tracks) {
            if (track.node.node_id == id) {
                track.node.version++;
                break;
            }
        }
    }
}

} // namespace tachyon

