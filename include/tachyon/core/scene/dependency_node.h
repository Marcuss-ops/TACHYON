#pragma once

#include <cstdint>
#include <vector>
#include <atomic>

namespace tachyon::core::scene {

/**
 * @brief Base class for nodes in the dependency graph.
 * Uses a push/pull model with revision numbers (ticks) for invalidation.
 */
class DependencyNode {
public:
    DependencyNode() = default;
    DependencyNode(const DependencyNode& other)
        : m_last_modified_tick(other.m_last_modified_tick.load(std::memory_order_relaxed)),
          m_last_evaluated_tick(other.m_last_evaluated_tick.load(std::memory_order_relaxed)),
          m_parents(other.m_parents) {}

    DependencyNode& operator=(const DependencyNode& other) {
        if (this != &other) {
            m_last_modified_tick.store(other.m_last_modified_tick.load(std::memory_order_relaxed), std::memory_order_relaxed);
            m_last_evaluated_tick.store(other.m_last_evaluated_tick.load(std::memory_order_relaxed), std::memory_order_relaxed);
            m_parents = other.m_parents;
        }
        return *this;
    }

    virtual ~DependencyNode() = default;

    /**
     * @brief Marks this node and all its parents as dirty for the given tick.
     * @param tick The global edit tick/revision number.
     */
    void mark_dirty(uint64_t tick) {
        if (m_last_modified_tick >= tick) return;
        
        m_last_modified_tick = tick;
        
        // Propagate push to parents
        for (auto* parent : m_parents) {
            parent->mark_dirty(tick);
        }
    }

    /**
     * @brief Checks if the node is clean relative to its current modifications.
     */
    [[nodiscard]] bool is_clean() const {
        return m_last_evaluated_tick >= m_last_modified_tick;
    }

    /**
     * @brief Records that the node has been successfully evaluated for the current revision.
     * @param tick The tick at which evaluation completed.
     */
    void mark_evaluated(uint64_t tick) {
        m_last_evaluated_tick = tick;
    }

    void add_parent(DependencyNode* parent) {
        if (parent) {
            m_parents.push_back(parent);
        }
    }

    [[nodiscard]] uint64_t last_modified_tick() const { return m_last_modified_tick; }
    [[nodiscard]] uint64_t last_evaluated_tick() const { return m_last_evaluated_tick; }

protected:
    std::atomic<uint64_t> m_last_modified_tick{0};
    std::atomic<uint64_t> m_last_evaluated_tick{0};
    std::vector<DependencyNode*> m_parents;
};

} // namespace tachyon::core::scene
