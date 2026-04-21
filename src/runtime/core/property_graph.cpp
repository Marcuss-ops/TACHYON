#include "tachyon/runtime/core/property_graph.h"

namespace tachyon {

std::uint32_t PropertyGraph::add_node(PropertyNode node) {
    if (m_nodes.size() >= 64) {
        // Hard limit due to dependency_mask being 64-bit
        throw std::runtime_error("PropertyGraph limit exceeded: Tachyon currently supports a maximum of 64 property nodes per composition due to dependency mask constraints.");
    }
    node.id = static_cast<std::uint32_t>(m_nodes.size());
    m_nodes.push_back(node);
    return node.id;
}

void PropertyGraph::add_dependency(std::uint32_t from, std::uint32_t to) {
    if (from >= m_nodes.size() || to >= m_nodes.size()) {
        throw std::out_of_range("PropertyGraph node index out of range");
    }

    m_nodes[from].affects.push_back(to);
    m_nodes[to].depends_on.push_back(from);
}

const std::vector<std::uint32_t>& PropertyGraph::dependencies(std::uint32_t node_id) const {
    static const std::vector<std::uint32_t> empty;
    if (node_id >= m_nodes.size()) {
        return empty;
    }
    return m_nodes[node_id].depends_on;
}

const std::vector<std::uint32_t>& PropertyGraph::affected(std::uint32_t node_id) const {
    static const std::vector<std::uint32_t> empty;
    if (node_id >= m_nodes.size()) {
        return empty;
    }
    return m_nodes[node_id].affects;
}

void PropertyGraph::clear() {
    m_nodes.clear();
}

} // namespace tachyon
