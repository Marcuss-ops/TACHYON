#pragma once
#include "tachyon/core/api.h"
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>

namespace tachyon {

/**
 * @brief Represents a node in the execution graph for a specific property or layer.
 * 
 * Nodes participate in a dynamic execution graph with adjacency list based tracking for 
 * efficient dependency resolution and dirty propagation.
 */
struct TACHYON_ALIGN(16) PropertyNode {
    std::uint32_t id{0};           ///< Unique identifier within the composition.
    std::uint32_t track_index{0};  ///< Index of the associated animation track.
    std::uint32_t version{0};      ///< Version counter for dirty tracking.
    std::uint64_t hash{0};         ///< Current value hash for change detection.
    
    std::vector<std::uint32_t> depends_on; ///< Indices of nodes this node depends on.
    std::vector<std::uint32_t> affects;    ///< Indices of nodes affected by this node.
};

class PropertyGraph {
public:
    [[nodiscard]] std::uint32_t add_node(PropertyNode node);
    void add_dependency(std::uint32_t from, std::uint32_t to);

    [[nodiscard]] const std::vector<PropertyNode>& nodes() const noexcept {
        return m_nodes;
    }

    [[nodiscard]] const std::vector<std::uint32_t>& dependencies(std::uint32_t node_id) const;
    [[nodiscard]] const std::vector<std::uint32_t>& affected(std::uint32_t node_id) const;

    void clear();

private:
    std::vector<PropertyNode> m_nodes;
};

} // namespace tachyon
