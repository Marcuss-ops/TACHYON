#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace tachyon::runtime {

enum class NodeType {
    Property,
    Layer,
    Composition,
    Resource, // e.g. a loaded image or font
    Global    // e.g. time, global expressions
};

struct DependencyNode {
    std::string id;       // Unique identifier (e.g. "comp[0].layer[1].transform.position")
    NodeType type;
    std::uint64_t hash;   // Hash of the current evaluated state of this node
    bool is_dirty{true};  // True if needs re-evaluation

    std::vector<std::string> depends_on;  // Nodes this node reads from
    std::vector<std::string> dependents;  // Nodes that read from this node
};

class DependencyGraph {
public:
    DependencyGraph() = default;
    ~DependencyGraph() = default;

    void add_node(const std::string& id, NodeType type);
    void add_dependency(const std::string& dependent_id, const std::string& provider_id);

    // Mark a node and all its transitive dependents as dirty
    void invalidate(const std::string& id);

    // Update the hash of a node and clear its dirty flag.
    // Returns true if the hash changed. If it did change, its dependents might need invalidation.
    bool update_node(const std::string& id, std::uint64_t new_hash);

    bool is_dirty(const std::string& id) const;
    const DependencyNode* get_node(const std::string& id) const;

    // Get an ordered list of nodes that need evaluation, topologically sorted
    std::vector<std::string> get_evaluation_order() const;

    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<DependencyNode>> m_nodes;

    void mark_dirty_recursive(DependencyNode* node, std::unordered_set<std::string>& visited);
};

} // namespace tachyon::runtime
