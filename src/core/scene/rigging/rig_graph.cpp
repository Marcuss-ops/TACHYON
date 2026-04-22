#include "tachyon/core/scene/rigging/rig_graph.h"
#include "tachyon/core/math/quaternion.h"
#include <unordered_set>
#include <queue>

namespace tachyon::scene::rigging {

void Pose::add_node(const PoseNode& node) {
    if (m_id_to_index.find(node.id) == m_id_to_index.end()) {
        m_id_to_index[node.id] = m_nodes.size();
        m_nodes.push_back(node);
    } else {
        m_nodes[m_id_to_index[node.id]] = node;
    }
    m_hierarchy_dirty = true;
}

PoseNode* Pose::get_node(const std::string& id) {
    auto it = m_id_to_index.find(id);
    if (it != m_id_to_index.end()) {
        return &m_nodes[it->second];
    }
    return nullptr;
}

const PoseNode* Pose::get_node(const std::string& id) const {
    auto it = m_id_to_index.find(id);
    if (it != m_id_to_index.end()) {
        return &m_nodes[it->second];
    }
    return nullptr;
}

void Pose::rebuild_hierarchy() {
    if (!m_hierarchy_dirty) return;
    
    for (auto& node : m_nodes) {
        node.parent_index = -1;
        node.children_indices.clear();
    }

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (!m_nodes[i].parent_id.empty()) {
            auto it = m_id_to_index.find(m_nodes[i].parent_id);
            if (it != m_id_to_index.end()) {
                m_nodes[i].parent_index = static_cast<int>(it->second);
                m_nodes[it->second].children_indices.push_back(static_cast<int>(i));
            }
        }
    }
    m_hierarchy_dirty = false;
}

void Pose::compute_world_matrices() {
    rebuild_hierarchy();

    for (auto& node : m_nodes) {
        node.local_matrix = math::compose_trs(node.local_position, node.local_rotation, node.local_scale);
    }

    // Process nodes such that parents are always processed before children.
    // A simple way is to use a queue starting from roots.
    std::vector<int> stack;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].parent_index == -1) {
            stack.push_back(static_cast<int>(i));
        }
    }

    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();
        
        auto& node = m_nodes[idx];
        if (node.parent_index == -1) {
            node.world_matrix = node.local_matrix;
        } else {
            node.world_matrix = m_nodes[node.parent_index].world_matrix * node.local_matrix;
        }

        node.world_position = {node.world_matrix[12], node.world_matrix[13], node.world_matrix[14]};
        node.world_scale = {
            math::Vector3(node.world_matrix[0], node.world_matrix[4], node.world_matrix[8]).length(),
            math::Vector3(node.world_matrix[1], node.world_matrix[5], node.world_matrix[9]).length(),
            math::Vector3(node.world_matrix[2], node.world_matrix[6], node.world_matrix[10]).length()
        };
        node.world_normal = math::Vector3(node.world_matrix[8], node.world_matrix[9], node.world_matrix[10]).normalized();

        for (int child_idx : node.children_indices) {
            stack.push_back(child_idx);
        }
    }
}

void Pose::propagate_world_matrix(int node_index) {
    if (node_index < 0 || node_index >= static_cast<int>(m_nodes.size())) return;
    
    const auto& node = m_nodes[node_index];
    for (int child_idx : node.children_indices) {
        auto& child = m_nodes[child_idx];
        child.world_matrix = node.world_matrix * child.local_matrix;
        child.world_position = {child.world_matrix[12], child.world_matrix[13], child.world_matrix[14]};
        child.world_scale = {
            math::Vector3(child.world_matrix[0], child.world_matrix[4], child.world_matrix[8]).length(),
            math::Vector3(child.world_matrix[1], child.world_matrix[5], child.world_matrix[9]).length(),
            math::Vector3(child.world_matrix[2], child.world_matrix[6], child.world_matrix[10]).length()
        };
        child.world_normal = math::Vector3(child.world_matrix[8], child.world_matrix[9], child.world_matrix[10]).normalized();
        propagate_world_matrix(child_idx);
    }
}

void RigGraph::build_from_layers(const std::vector<scene::EvaluatedLayerState>& layers) {
    m_constraints.clear();
    m_ik_tasks.clear();
    m_topological_order.clear();

    for (const auto& layer : layers) {
        for (const auto& constraint : layer.constraints) {
            m_constraints.push_back({layer.id, constraint});
        }
        for (const auto& ik : layer.ik_chains) {
            m_ik_tasks.push_back({layer.id, ik});
        }
    }

    topological_sort(layers);
}

void RigGraph::topological_sort(const std::vector<scene::EvaluatedLayerState>& layers) {
    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, int> in_degree;

    for (const auto& layer : layers) {
        in_degree[layer.id] = 0;
    }

    for (const auto& layer : layers) {
        // Constraints dependencies
        for (const auto& constraint : layer.constraints) {
            for (const auto& target : constraint.targets) {
                adj[target.layer_id].push_back(layer.id);
                in_degree[layer.id]++;
            }
        }
        // IK dependencies
        for (const auto& ik : layer.ik_chains) {
            adj[ik.target_id].push_back(layer.id);
            in_degree[layer.id]++;
            if (ik.use_pole_vector && !ik.pole_vector_id.empty()) {
                adj[ik.pole_vector_id].push_back(layer.id);
                in_degree[layer.id]++;
            }
        }
    }

    std::queue<std::string> q;
    for (const auto& pair : in_degree) {
        if (pair.second == 0) {
            q.push(pair.first);
        }
    }

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        m_topological_order.push_back(current);

        for (const auto& neighbor : adj[current]) {
            in_degree[neighbor]--;
            if (in_degree[neighbor] == 0) {
                q.push(neighbor);
            }
        }
    }

    // Cycle detection
    for (const auto& pair : in_degree) {
        if (pair.second > 0) {
            // Cycle detected! For now, just add them to the end
            bool already_added = false;
            for (const auto& id : m_topological_order) if (id == pair.first) { already_added = true; break; }
            if (!already_added) m_topological_order.push_back(pair.first);
        }
    }
}

void RigGraph::evaluate(Pose& pose) const {
    // Phase 1: Forward Kinematics (FK)
    // Computes initial world matrices from animated/authored local TRS.
    pose.compute_world_matrices();

    // Phase 2: Constraints Solve
    // Solves Point, Orient, Parent, and LookAt constraints in topological order.
    solve_constraints(pose);

    // Phase 3: Inverse Kinematics (IK) Solve
    // Applies IK solvers (e.g., CCD) to defined chains.
    solve_ik(pose);
}

void RigGraph::solve_constraints(Pose& pose) const {
    for (const auto& id : m_topological_order) {
        PoseNode* node = pose.get_node(id);
        if (!node) continue;

        for (const auto& rig_constraint : m_constraints) {
            if (rig_constraint.source_id != id) continue;
            const auto& constraint = rig_constraint.spec;
            if (constraint.targets.empty() || constraint.weight <= 0.0f) continue;

            if (constraint.type == scene::ConstraintType::Point) {
                math::Vector3 target_pos = math::Vector3::zero();
                float total_weight = 0.0f;
                for (const auto& target : constraint.targets) {
                    const PoseNode* target_node = pose.get_node(target.layer_id);
                    if (target_node) {
                        target_pos = target_pos + (math::Vector3{target_node->world_matrix[12], target_node->world_matrix[13], target_node->world_matrix[14]} + target.offset) * target.weight;
                        total_weight += target.weight;
                    }
                }
                if (total_weight > 0.0f) {
                    target_pos = target_pos / total_weight;
                    math::Vector3 current_pos = {node->world_matrix[12], node->world_matrix[13], node->world_matrix[14]};
                    math::Vector3 blended_pos = current_pos * (1.0f - constraint.weight) + target_pos * constraint.weight;
                    node->world_matrix[12] = blended_pos.x;
                    node->world_matrix[13] = blended_pos.y;
                    node->world_matrix[14] = blended_pos.z;
                }
            } else if (constraint.type == scene::ConstraintType::Orient) {
                const auto& target = constraint.targets[0];
                const PoseNode* target_node = pose.get_node(target.layer_id);
                if (target_node) {
                    const auto& m = target_node->world_matrix;
                    math::Vector3 sx = {m[0], m[4], m[8]};
                    math::Vector3 sy = {m[1], m[5], m[9]};
                    math::Vector3 sz = {m[2], m[6], m[10]};
                    sx = sx.normalized(); sy = sy.normalized(); sz = sz.normalized();
                    
                    math::Matrix4x4 r;
                    r[0]=sx.x; r[1]=sy.x; r[2]=sz.x; r[3]=0;
                    r[4]=sx.y; r[5]=sy.y; r[6]=sz.y; r[7]=0;
                    r[8]=sx.z; r[9]=sy.z; r[10]=sz.z; r[11]=0;
                    r[12]=node->world_matrix[12]; r[13]=node->world_matrix[13]; r[14]=node->world_matrix[14]; r[15]=1;
                    
                    const float w = constraint.weight * target.weight;
                    for (int i = 0; i < 16; ++i) {
                        node->world_matrix[i] = node->world_matrix[i] * (1.0f - w) + r[i] * w;
                    }
                }
            } else if (constraint.type == scene::ConstraintType::Parent) {
                const auto& target = constraint.targets[0];
                const PoseNode* target_node = pose.get_node(target.layer_id);
                if (target_node) {
                    math::Matrix4x4 offset_t = math::Matrix4x4::translation(target.offset);
                    math::Matrix4x4 constrained_matrix = target_node->world_matrix * offset_t;
                    const float w = constraint.weight * target.weight;
                    for (int i = 0; i < 16; ++i) {
                        node->world_matrix[i] = node->world_matrix[i] * (1.0f - w) + constrained_matrix[i] * w;
                    }
                }
            } else if (constraint.type == scene::ConstraintType::LookAt) {
                const auto& target = constraint.targets[0];
                const PoseNode* target_node = pose.get_node(target.layer_id);
                if (target_node) {
                    math::Vector3 target_pos = {target_node->world_matrix[12], target_node->world_matrix[13], target_node->world_matrix[14]};
                    math::Vector3 current_pos = {node->world_matrix[12], node->world_matrix[13], node->world_matrix[14]};
                    const math::Vector3 direction = (target_pos - current_pos);
                    if (direction.length_squared() > 1e-6f) {
                        const math::Quaternion look_rot = math::Quaternion::look_at(current_pos, target_pos, {0, 1, 0});
                        math::Matrix4x4 r = look_rot.to_matrix();
                        r[12] = current_pos.x; r[13] = current_pos.y; r[14] = current_pos.z;
                        
                        const float w = constraint.weight;
                        for (int i = 0; i < 16; ++i) {
                            node->world_matrix[i] = node->world_matrix[i] * (1.0f - w) + r[i] * w;
                        }
                    }
                }
            }
        }
    }
}

static void rotate_hierarchy_fast(Pose& pose, int joint_idx, const math::Matrix4x4& rot_matrix) {
    auto& nodes = pose.nodes();
    nodes[joint_idx].world_matrix = rot_matrix * nodes[joint_idx].world_matrix;
    nodes[joint_idx].world_position = {nodes[joint_idx].world_matrix[12], nodes[joint_idx].world_matrix[13], nodes[joint_idx].world_matrix[14]};
    pose.propagate_world_matrix(joint_idx);
}

void RigGraph::solve_ik(Pose& pose) const {
    auto& nodes = pose.nodes();
    for (const auto& task : m_ik_tasks) {
        const auto& spec = task.spec;
        if (spec.weight <= 0.0f) continue;

        PoseNode* end_effector = pose.get_node(task.end_effector_id);
        const PoseNode* target = pose.get_node(spec.target_id);
        if (!end_effector || !target) continue;

        math::Vector3 target_pos = {target->world_matrix[12], target->world_matrix[13], target->world_matrix[14]};

        // Build chain of indices
        std::vector<int> chain_indices;
        auto* current = end_effector;
        for (int i = 0; i < spec.chain_length && current; ++i) {
            auto it = pose.nodes().begin();
            // This is still a bit slow, but better if we had indices in the spec
            // For now, we find index of current
            int idx = -1;
            for(size_t j=0; j<nodes.size(); ++j) if(&nodes[j] == current) { idx = static_cast<int>(j); break; }
            if (idx == -1) break;

            chain_indices.push_back(idx);
            if (current->parent_index == -1) break;
            current = &nodes[current->parent_index];
        }

        if (chain_indices.size() < 2) continue;

        // CCD Iterations
        for (int iter = 0; iter < spec.iterations; ++iter) {
            for (size_t i = 1; i < chain_indices.size(); ++i) {
                int joint_idx = chain_indices[i];
                auto& joint = nodes[joint_idx];
                
                math::Vector3 effector_pos = {end_effector->world_matrix[12], end_effector->world_matrix[13], end_effector->world_matrix[14]};
                math::Vector3 joint_pos = {joint.world_matrix[12], joint.world_matrix[13], joint.world_matrix[14]};

                if ((effector_pos - target_pos).length_squared() < 1e-6f) break;

                math::Vector3 j_to_e = (effector_pos - joint_pos).normalized();
                math::Vector3 j_to_t = (target_pos - joint_pos).normalized();

                float dot = math::Vector3::dot(j_to_e, j_to_t);
                if (dot < 0.99999f) {
                    math::Vector3 axis = math::Vector3::cross(j_to_e, j_to_t);
                    if (axis.length_squared() > 1e-9f) {
                        axis = axis.normalized();
                        float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));
                        
                        math::Quaternion delta_rot = math::Quaternion::from_axis_angle(axis, angle * spec.weight);
                        math::Matrix4x4 rot_matrix = math::Matrix4x4::translation(joint_pos) * delta_rot.to_matrix() * math::Matrix4x4::translation(-joint_pos);
                        
                        rotate_hierarchy_fast(pose, joint_idx, rot_matrix);
                    }
                }
            }
        }
    }
}

void RigGraph::apply_to_layers(const Pose& solved_pose, std::vector<scene::EvaluatedLayerState>& layers) const {
    for (auto& layer : layers) {
        const PoseNode* node = solved_pose.get_node(layer.id);
        if (node) {
            layer.world_matrix = node->world_matrix;
            layer.world_position3 = {node->world_matrix[12], node->world_matrix[13], node->world_matrix[14]};
            layer.world_normal = math::Vector3(node->world_matrix[8], node->world_matrix[9], node->world_matrix[10]).normalized();
        }
    }
}

} // namespace tachyon::scene::rigging
