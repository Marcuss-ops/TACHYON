#include "tachyon/runtime/core/render_graph.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace tachyon {

void RenderGraph::add_edge(std::uint32_t from, std::uint32_t to, bool structural) {
    m_edges.push_back({from, to, structural});
    m_nodes.insert(from);
    m_nodes.insert(to);
}

void RenderGraph::add_node(std::uint32_t node_id) {
    m_nodes.insert(node_id);
}

void RenderGraph::compile() {
    // 1. Identify all unique nodes and build adjacency list
    std::map<std::uint32_t, std::vector<std::uint32_t>> adj;
    std::set<std::uint32_t> all_nodes = m_nodes;

    for (const auto& edge : m_edges) {
        adj[edge.from].push_back(edge.to);
        all_nodes.insert(edge.from);
        all_nodes.insert(edge.to);
    }
    
    // 2. Sort adjacency lists to ensure deterministic traversal order
    for (auto& [node, neighbors] : adj) {
        std::sort(neighbors.begin(), neighbors.end());
    }

    m_topo_order.clear();
    m_topo_order.reserve(all_nodes.size());
    
    std::set<std::uint32_t> visited;
    std::set<std::uint32_t> path;

    // DFS for topological sort
    auto visit = [&](auto self, std::uint32_t n) -> void {
        if (visited.count(n)) return;
        if (path.count(n)) {
            throw std::runtime_error("Circular dependency detected in RenderGraph at node " + std::to_string(n));
        }
        
        path.insert(n);
        
        auto it = adj.find(n);
        if (it != adj.end()) {
            for (std::uint32_t m : it->second) {
                self(self, m);
            }
        }
        
        path.erase(n);
        visited.insert(n);
        m_topo_order.push_back(n);
    };


    // 3. Process nodes in sorted order for determinism
    for (std::uint32_t n : all_nodes) {
        if (visited.count(n) == 0) {
            visit(visit, n);
        }
    }

    // 4. Reverse to get final topological order
    std::reverse(m_topo_order.begin(), m_topo_order.end());
}

void RenderGraph::increment_node_version(std::uint32_t node_id, std::uint32_t& out_new_version) {
    // This method is called when a leaf node (property/asset) changes.
    // The implementation of version storage is outside this class, 
    // but we provide the logic to find all impacted dependents.
    std::vector<std::uint32_t> affected;
    propagate_invalidation(node_id, affected);
    
    // The caller will increment the version of all nodes in 'affected'.
    out_new_version = 0; // Placeholder for the root's new version
}

void RenderGraph::propagate_invalidation(std::uint32_t changed_node_id, std::vector<std::uint32_t>& invalidated_nodes) {
    std::vector<std::uint32_t> stack;
    stack.push_back(changed_node_id);

    // Build dependent adjacency (inverted from edges list)
    std::map<std::uint32_t, std::vector<std::uint32_t>> dependents_map;
    for (const auto& edge : m_edges) {
        dependents_map[edge.from].push_back(edge.to);
    }

    std::set<std::uint32_t> visited;
    while (!stack.empty()) {
        std::uint32_t current = stack.back();
        stack.pop_back();

        if (visited.count(current)) continue;
        visited.insert(current);
        invalidated_nodes.push_back(current);

        auto it = dependents_map.find(current);
        if (it != dependents_map.end()) {
            for (std::uint32_t dep : it->second) {
                stack.push_back(dep);
            }
        }
    }
}

void RenderGraph::clear() {
    m_edges.clear();
    m_nodes.clear();
    m_topo_order.clear();
}



} // namespace tachyon
