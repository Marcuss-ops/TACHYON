#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>
#include <stdexcept>

namespace tachyon {

struct PropertyNode {
    std::uint32_t id{0};
    std::uint32_t track_index{0};
    std::vector<std::uint32_t> depends_on;
    std::vector<std::uint32_t> affects;
};

class PropertyGraph {
public:
    [[nodiscard]] std::uint32_t add_node(PropertyNode node) {
        if (m_nodes.size() >= 64) {
            // Hard limit due to dependency_mask being 64-bit
            throw std::runtime_error("PropertyGraph limit exceeded: Tachyon currently supports a maximum of 64 property nodes per composition due to dependency mask constraints.");
        }
        node.id = static_cast<std::uint32_t>(m_nodes.size());
        m_nodes.push_back(node);
        return node.id;
    }

    void add_dependency(std::uint32_t from, std::uint32_t to) {
        if (from >= m_nodes.size() || to >= m_nodes.size()) {
            throw std::out_of_range("PropertyGraph node index out of range");
        }

        

        m_nodes[from].affects.push_back(to);
        m_nodes[to].depends_on.push_back(from);
    }

    [[nodiscard]] const std::vector<PropertyNode>& nodes() const noexcept {
        return m_nodes;
    }

    [[nodiscard]] const std::vector<std::uint32_t>& dependencies(std::uint32_t node_id) const {
        static const std::vector<std::uint32_t> empty;
        if (node_id >= m_nodes.size()) {
            return empty;
        }
        return m_nodes[node_id].depends_on;
    }

    [[nodiscard]] const std::vector<std::uint32_t>& affected(std::uint32_t node_id) const {
        static const std::vector<std::uint32_t> empty;
        if (node_id >= m_nodes.size()) {
            return empty;
        }
        return m_nodes[node_id].affects;
    }

    void clear() {
        m_nodes.clear();
    }

private:
    std::vector<PropertyNode> m_nodes;
};

} // namespace tachyon
