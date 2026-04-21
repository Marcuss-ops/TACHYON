#include "tachyon/runtime/core/render_graph.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace tachyon {

void RenderGraph::add_edge(std::uint32_t from, std::uint32_t to, bool structural) {
    m_edges.push_back({from, to, structural});
}

void RenderGraph::compile() {
    // 1. Identify all unique nodes and build adjacency list
    std::map<std::uint32_t, std::vector<std::uint32_t>> adj;
    std::set<std::uint32_t> all_nodes;
    
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
            // Cycle detected - in a production engine we should handle or report this.
            return;
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

void RenderGraph::clear() {
    m_edges.clear();
    m_topo_order.clear();
}

} // namespace tachyon
