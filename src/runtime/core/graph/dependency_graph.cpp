#include "tachyon/runtime/core/graph/dependency_graph.h"
#include <algorithm>
#include <queue>

namespace tachyon::runtime {

void DependencyGraph::add_node(const std::string& id, NodeType type) {
    if (m_nodes.find(id) == m_nodes.end()) {
        auto node = std::make_unique<DependencyNode>();
        node->id = id;
        node->type = type;
        node->hash = 0;
        node->is_dirty = true;
        m_nodes[id] = std::move(node);
    }
}

void DependencyGraph::add_dependency(const std::string& dependent_id, const std::string& provider_id) {
    if (m_nodes.find(dependent_id) != m_nodes.end() && m_nodes.find(provider_id) != m_nodes.end()) {
        m_nodes[dependent_id]->depends_on.push_back(provider_id);
        m_nodes[provider_id]->dependents.push_back(dependent_id);
    }
}

void DependencyGraph::invalidate(const std::string& id) {
    auto it = m_nodes.find(id);
    if (it != m_nodes.end()) {
        std::unordered_set<std::string> visited;
        mark_dirty_recursive(it->second.get(), visited);
    }
}

void DependencyGraph::mark_dirty_recursive(DependencyNode* node, std::unordered_set<std::string>& visited) {
    if (!node) return;
    if (!visited.insert(node->id).second) return;
    node->is_dirty = true;
    for (const auto& dep_id : node->dependents) {
        auto it = m_nodes.find(dep_id);
        if (it != m_nodes.end()) {
            mark_dirty_recursive(it->second.get(), visited);
        }
    }
}

bool DependencyGraph::update_node(const std::string& id, std::uint64_t new_hash) {
    auto it = m_nodes.find(id);
    if (it != m_nodes.end()) {
        bool changed = (it->second->hash != new_hash);
        it->second->hash = new_hash;
        it->second->is_dirty = false;
        
        if (changed) {
            // Hash changed, so dependents must remain dirty for re-evaluation
            for (const auto& dep_id : it->second->dependents) {
                invalidate(dep_id);
            }
        }
        return changed;
    }
    return false;
}

bool DependencyGraph::is_dirty(const std::string& id) const {
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? it->second->is_dirty : true; // Missing nodes are considered dirty
}

const DependencyNode* DependencyGraph::get_node(const std::string& id) const {
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> DependencyGraph::get_evaluation_order() const {
    std::vector<std::string> order;
    std::unordered_map<std::string, std::size_t> indegree;

    for (const auto& [id, node] : m_nodes) {
        if (!node->is_dirty) {
            continue;
        }
        indegree.emplace(id, 0);
    }

    for (const auto& [id, node] : m_nodes) {
        if (!node->is_dirty) {
            continue;
        }
        for (const auto& dep_id : node->depends_on) {
            auto dep_it = m_nodes.find(dep_id);
            if (dep_it != m_nodes.end() && dep_it->second->is_dirty) {
                indegree[id]++;
            }
        }
    }

    std::queue<std::string> ready;
    for (const auto& [id, degree] : indegree) {
        if (degree == 0) {
            ready.push(id);
        }
    }

    std::unordered_set<std::string> visited;
    while (!ready.empty()) {
        const std::string current = ready.front();
        ready.pop();
        if (!visited.insert(current).second) {
            continue;
        }
        order.push_back(current);

        auto node_it = m_nodes.find(current);
        if (node_it == m_nodes.end()) {
            continue;
        }

        for (const auto& dependent_id : node_it->second->dependents) {
            auto dep_it = indegree.find(dependent_id);
            if (dep_it == indegree.end()) {
                continue;
            }
            if (dep_it->second > 0) {
                --dep_it->second;
                if (dep_it->second == 0) {
                    ready.push(dependent_id);
                }
            }
        }
    }

    for (const auto& [id, node] : m_nodes) {
        if (node->is_dirty && std::find(order.begin(), order.end(), id) == order.end()) {
            order.push_back(id);
        }
    }

    return order;
}

void DependencyGraph::clear() {
    m_nodes.clear();
}

} // namespace tachyon::runtime
