#include "tachyon/core/spec/compilation/scene_compiler_detail.h"
#include <unordered_map>
#include <cstdint>

namespace tachyon::detail {

void resolve_dependencies(const SceneSpec& scene, CompiledScene& compiled, CompilationRegistry& registry) {
    for (std::uint32_t c_idx = 0; c_idx < compiled.compositions.size(); ++c_idx) {
        auto& comp = compiled.compositions[c_idx];
        const auto& comp_spec = scene.compositions[c_idx];
        
        for (std::uint32_t l_idx = 0; l_idx < comp.layers.size(); ++l_idx) {
            auto& layer = comp.layers[l_idx];
            
            // Expanded layers from components are added to comp.layers but not to comp_spec.layers.
            // For now, we only resolve dependencies for original layers.
            // TODO: Store dependency info during layer compilation in registry to handle expanded layers.
            if (l_idx >= comp_spec.layers.size()) continue;
            
            const auto& layer_spec = comp_spec.layers[l_idx];

            if (layer_spec.parent.has_value()) {
                auto it = registry.layer_id_map.find(*layer_spec.parent);
                if (it != registry.layer_id_map.end()) {
                    layer.parent_index = it->second;
                    compiled.graph.add_edge(comp.layers[it->second].node.node_id, layer.node.node_id, true);
                }
            }

            if (layer_spec.track_matte_layer_id.has_value()) {
                auto it = registry.layer_id_map.find(*layer_spec.track_matte_layer_id);
                if (it != registry.layer_id_map.end()) {
                    layer.matte_layer_index = it->second;
                    compiled.graph.add_edge(comp.layers[it->second].node.node_id, layer.node.node_id, true);
                }
            }

            if (layer_spec.precomp_id.has_value()) {
                auto it = registry.composition_id_map.find(*layer_spec.precomp_id);
                if (it != registry.composition_id_map.end()) {
                    layer.precomp_index = it->second;
                    compiled.graph.add_edge(compiled.compositions[it->second].node.node_id, layer.node.node_id, true);
                } else {
                    // Warning is added during layer compilation, skip here
                }
            }
        }
    }
}

} // namespace tachyon::detail
