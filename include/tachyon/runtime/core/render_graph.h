#pragma once

#include <cstdint>
#include <set>
#include <vector>

namespace tachyon {

/**
 * @brief Manages the execution order and dependency relationships of the compiled scene.
 * 
 * The RenderGraph is built once during scene compilation and used by the 
 * FrameExecutor to determine the minimal set of nodes that need evaluation.
 */
class RenderGraph {
public:
    /**
     * @brief Represents a dependency edge in the graph.
     */
    struct Edge {
        std::uint32_t from;
        std::uint32_t to;
        bool structural{true}; ///< True if structural (parent, matte), false if temporal (expressions).
    };

    /**
     * @brief Adds a dependency edge between nodes.
     * @param from The source node ID (dependency).
     * @param to The target node ID (dependent).
     * @param structural True if structural, false if temporal.
     */
    void add_edge(std::uint32_t from, std::uint32_t to, bool structural = true);

    /**
     * @brief Registers a node in the graph even if it has no incident edges.
     * 
     * This is required for root compositions and other isolated nodes that still
     * need to participate in topological ordering and execution.
     */
    void add_node(std::uint32_t node_id);
    
    /**
     * @brief Performs a deterministic topological sort of the graph.
     * This establishes the m_topo_order used during execution.
     */
    void compile();

    /**
     * @brief Returns the nodes sorted in topological order.
     */
    [[nodiscard]] const std::vector<std::uint32_t>& topo_order() const noexcept { return m_topo_order; }
    
    /**
     * @brief Returns all edges in the graph.
     */
    [[nodiscard]] const std::vector<Edge>& edges() const noexcept { return m_edges; }

    /**
     * @brief Clears the graph state.
     */
    void clear();

    /**
     * @brief Propagates invalidation from a changed node through dependents.
     */
    void propagate_invalidation(std::uint32_t changed_node_id, std::vector<std::uint32_t>& invalidated_nodes);

    /**
     * @brief Increments the version of a node and records invalidated dependents.
     */
    void increment_node_version(std::uint32_t node_id, std::uint32_t& out_new_version);

private:
    std::vector<Edge> m_edges;
    std::set<std::uint32_t> m_nodes;
    std::vector<std::uint32_t> m_topo_order;
};

} // namespace tachyon
