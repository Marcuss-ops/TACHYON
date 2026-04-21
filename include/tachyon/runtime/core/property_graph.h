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
