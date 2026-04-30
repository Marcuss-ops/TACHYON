#pragma once

#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/scene/constraints/constraints.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon::scene {
struct EvaluatedLayerState;
}

namespace tachyon::scene::rigging {

struct PoseNode {
    std::string id;
    std::string parent_id;
    int parent_index{-1};
    std::vector<int> children_indices;
    
    // Inputs from Animation/FK
    math::Vector3 local_position{0,0,0};
    math::Quaternion local_rotation{0,0,0,1};
    math::Vector3 local_scale{1,1,1};
    
    // Evaluated state
    math::Matrix4x4 local_matrix{math::Matrix4x4::identity()};
    math::Matrix4x4 world_matrix{math::Matrix4x4::identity()};
    math::Vector3 world_position{0,0,0};
    math::Vector3 world_scale{1,1,1};
    math::Vector3 world_normal{0,0,1};
};

class Pose {
public:
    void add_node(const PoseNode& node);
    PoseNode* get_node(const std::string& id);
    const PoseNode* get_node(const std::string& id) const;
    
    std::vector<PoseNode>& nodes() { return m_nodes; }
    const std::vector<PoseNode>& nodes() const { return m_nodes; }

    // Computes world matrices from local TRS and hierarchy
    void compute_world_matrices();
    
    // Propagates a world matrix change down the hierarchy
    void propagate_world_matrix(int node_index);

private:
    void rebuild_hierarchy();
    
    std::vector<PoseNode> m_nodes;
    std::unordered_map<std::string, std::size_t> m_id_to_index;
    bool m_hierarchy_dirty{true};
};

class RigGraph {
public:
    void build_from_layers(const std::vector<scene::EvaluatedLayerState>& layers);
    
    // Multi-pass evaluation
    void evaluate(Pose& current_pose) const;

    void apply_to_layers(const Pose& solved_pose, std::vector<scene::EvaluatedLayerState>& layers) const;

private:
    struct RigConstraint {
        std::string source_id;
        scene::ConstraintSpec spec;
    };

    struct RigIKTask {
        std::string end_effector_id;
        scene::IKSpec spec;
    };

    std::vector<std::string> m_topological_order;
    std::vector<RigConstraint> m_constraints;
    std::vector<RigIKTask> m_ik_tasks;

    void topological_sort(const std::vector<scene::EvaluatedLayerState>& layers);
    void solve_constraints(Pose& pose) const;
    void solve_ik(Pose& pose) const;
};

} // namespace tachyon::scene::rigging
