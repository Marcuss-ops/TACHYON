#include "tachyon/runtime/core/data/property_graph.h"

namespace tachyon {

std::uint32_t PropertyGraph::add_node(PropertyNode node) {
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

void PropertyGraph::invalidate_node(std::uint32_t node_id) {
    if (node_id >= m_nodes.size()) {
        return;
    }

    // Increment version for this node and all affected nodes
    std::vector<std::uint32_t> stack;
    stack.push_back(node_id);

    while (!stack.empty()) {
        const std::uint32_t current = stack.back();
        stack.pop_back();

        m_nodes[current].version++;

        for (std::uint32_t affected_id : m_nodes[current].affects) {
            stack.push_back(affected_id);
        }
    }
}

std::uint32_t PropertyGraph::get_version(std::uint32_t node_id) const {
    if (node_id >= m_nodes.size()) {
        return 0;
    }
    return m_nodes[node_id].version;
}

void PropertyGraph::clear() {
    m_nodes.clear();
}

} // namespace tachyon
