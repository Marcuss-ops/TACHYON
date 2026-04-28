#pragma once

#include "tachyon/runtime/core/data/compiled_scene.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace tachyon {

/**
 * @brief Context used during compilation to track node IDs and dependencies.
 */
struct CompilationRegistry {
    std::uint32_t next_id{1};
    std::unordered_map<std::string, std::uint32_t> layer_id_map;
    std::unordered_map<std::string, std::uint32_t> composition_id_map;
    
    CompiledNode create_node(CompiledNodeType type) {
        CompiledNode node;
        node.node_id = next_id++;
        node.type = type;
        return node;
    }
};

} // namespace tachyon
